import crcmod
import time
import struct
from typing import Optional, Dict, Any

# Message types
MSG_TYPE_SUMO_DATA = 0x01  # Python → STM32 (LaneCounts)
MSG_TYPE_LIGHT_STATE = 0x02  # STM32 → Python
MSG_TYPE_HEARTBEAT = 0x03  # Python → STM32 (optional)

# Frame structure offsets
OFFSET_SOF = 0  # bytes 0-1: 0xAA55
OFFSET_LEN = 2  # bytes 2-3: LEN field (little-endian)
OFFSET_TYPE = 4  # byte 4: TYPE field
HEADER_SIZE = 5  # SOF(2) + LEN(2) + TYPE(1)
CRC_SIZE = 2

# Protocol constants
SOF_PATTERN = bytes([0xAA, 0x55])
MAX_FRAME_LEN = 64  # Maximum frame size including SOF, LEN, TYPE, PAYLOAD, CRC
MAX_PAYLOAD_LEN = MAX_FRAME_LEN - HEADER_SIZE - CRC_SIZE  # 57 bytes

# Payload sizes (fixed)
SUMO_DATA_PAYLOAD_SIZE = 9  # ts_sec(4) + junction_type(1) + n,s,e,w(4)
LIGHT_STATE_PAYLOAD_SIZE = (
    4  # current_state(1) + decision_reason(1) + phase_duration(2)
)
HEARTBEAT_PAYLOAD_SIZE = 7  # uptime_ms(4) + seq(2) + status(1)


# Heartbeat logic
HEARTBEAT_INTERVAL = 2.0
HEARTBEAT_TIMEOUT = 6.0


def setup_crc16_ccitt():
    # crcmod uses 0x11021 (0x1021 shifted left by 3 bits)
    return crcmod.mkCrcFun(0x11021, initCrc=0xFFFF, rev=False, xorOut=0x0000)


# Global CRC function
crc16_ccitt = setup_crc16_ccitt()


class TLVOutputHandler:
    def __init__(self) -> None:
        # Receiver state (parsing STM32 → Python)
        self.state = "SEEKING_SOF"
        self.buffer = bytearray()
        self.expected_payload_len = 0
        self.current_type = 0
        self.current_payload: Optional[Dict[str, Any]] = None

        # Timing and connection monitoring
        self.start_time = time.time()
        self.last_heartbeat_tx = 0
        self.last_heartbeat_rx = time.time()

        # Counters for debugging/stats
        self.frames_ok = 0
        self.crc_fail = 0
        self.resyncs = 0

    # ================== TRANSMITTER (Python → STM32) ==================

    def create_lanecounts_frame(self, payload: Dict[str, Any]) -> bytes:
        # Create LaneCounts frame (0x01) for sending to STM32
        timestamp = payload["timestamp"]
        junction_type = payload["junction_type"]

        if junction_type not in ("x", "y"):
            raise ValueError(f"junction_type must be 'x' or 'y', got {junction_type}")

        jtype_num = 0 if junction_type == "x" else 1
        lanes = payload["lanes"]

        # Build payload: 9 bytes total
        payload_bytes = struct.pack(
            "<LBBBBB",
            timestamp,
            jtype_num,
            lanes["n"] & 0xFF,
            lanes["s"] & 0xFF,
            lanes["e"] & 0xFF,
            lanes["w"] & 0xFF,
        )

        return self._build_frame(MSG_TYPE_SUMO_DATA, payload_bytes)

    def _build_frame(self, msg_type: int, payload: bytes) -> bytes:
        # Build complete TLV frame with proper CRC coverage
        frame = bytearray()

        # 1. Start of Frame (SOF)
        frame.extend(SOF_PATTERN)

        # 2. LEN field: TYPE(1) + len(payload) + CRC(2)
        total_len = 1 + len(payload) + CRC_SIZE
        frame.extend(struct.pack("<H", total_len))

        # 3. TYPE field
        frame.append(msg_type)

        # 4. PAYLOAD
        frame.extend(payload)

        # 5. CRC over LEN(2) + TYPE(1) + PAYLOAD(n) bytes
        crc_data = frame[OFFSET_LEN:]  # From LEN byte onward
        crc_value = crc16_ccitt(bytes(crc_data))
        frame.extend(struct.pack("<H", crc_value))

        return bytes(frame)

    # ================== RECEIVER (STM32 → Python) ==================

    def process_bytes(self, data: bytes) -> list:
        # Process a chunk of bytes, return list of completed frames
        frames = []
        for byte in data:
            frame = self.process_byte(byte)
            if frame:
                frames.append(frame)
        return frames

    def process_byte(self, byte: bytes | int) -> Optional[Dict[str, Any]]:
        # Process a single received byte, return complete frame if available
        # Convert to integer
        if isinstance(byte, bytes):
            if len(byte) != 1:
                raise ValueError("process_byte expects a single byte")
            byte = byte[0]

        # Add to buffer
        self.buffer.append(byte)

        if len(self.buffer) > MAX_FRAME_LEN * 2:
            print("[TLV WARNING] Buffer overflow, resetting")
            self._reset_parser()
            return None

        # State machine
        if self.state == "SEEKING_SOF":
            self._seek_start_of_frame()
        elif self.state == "READING_HEADER":
            self._read_header()
        elif self.state == "READING_PAYLOAD":
            self._read_payload()
        elif self.state == "READING_CRC":
            return self._read_crc()

        return None

    def _seek_start_of_frame(self) -> None:
        # Look for SOF pattern at start of buffer
        if len(self.buffer) < 2:
            return

        # Check first two bytes
        if self.buffer[0] == 0xAA and self.buffer[1] == 0x55:
            self.state = "READING_HEADER"
        else:
            # Not SOF, discard first byte
            del self.buffer[0]
            self.resyncs += 1

    def _read_header(self) -> None:
        # Parse LEN and TYPE fields
        if len(self.buffer) < HEADER_SIZE:
            return

        try:
            # LEN field (little-endian)
            total_len = struct.unpack("<H", self.buffer[OFFSET_LEN:OFFSET_TYPE])[0]
        except struct.error:
            print("[TLV ERROR] Failed to unpack LEN field")
            self._reset_parser()
            return

        # Validate LEN: TYPE(1) + PAYLOAD + CRC(2)
        # Minimum: TYPE(1) + CRC(2) = 3
        # Maximum: based on payload size
        max_len_field = 1 + MAX_PAYLOAD_LEN + CRC_SIZE  # TYPE(1) + max_payload + CRC(2)
        if total_len < 3 or total_len > max_len_field:  # Reasonable upper bound
            print(f"[TLV ERROR] Invalid LEN: {total_len}")
            self._reset_parser()
            return

        self.current_type = self.buffer[OFFSET_TYPE]
        self.expected_payload_len = total_len - 3  # TYPE(1) + CRC(2)
        self.state = "READING_PAYLOAD"

    def _read_payload(self) -> None:
        # Wait for complete payload
        required_bytes = HEADER_SIZE + self.expected_payload_len
        if len(self.buffer) < required_bytes:
            return

        payload_bytes = self.buffer[HEADER_SIZE:required_bytes]

        # Parse based on message type
        if self.current_type == MSG_TYPE_LIGHT_STATE:
            self._parse_light_state(payload_bytes)
        elif self.current_type == MSG_TYPE_HEARTBEAT:
            self._parse_heartbeat(payload_bytes)
        else:
            print(f"[TLV ERROR] Unknown message type: 0x{self.current_type:02X}")
            self._reset_parser()
            return

        self.state = "READING_CRC"

    def _parse_light_state(self, payload_bytes: bytes) -> None:
        # Parse LightState message from STM32
        if len(payload_bytes) != LIGHT_STATE_PAYLOAD_SIZE:
            print(f"[TLV ERROR] LightState wrong size: {len(payload_bytes)}")
            self.current_payload = None
            return

        try:
            current_state = payload_bytes[0]
            decision_reason = payload_bytes[1]
            phase_duration = struct.unpack("<H", payload_bytes[2:4])[0]

            self.current_payload = {
                "current_state": current_state,
                "decision_reason": decision_reason,
                "phase_duration": phase_duration,
            }
        except struct.error:
            print("[TLV ERROR] Failed to parse LightState payload")
            self.current_payload = None

    def _parse_heartbeat(self, payload_bytes: bytes) -> None:
        # Parse Heartbeat message
        if len(payload_bytes) != HEARTBEAT_PAYLOAD_SIZE:
            print(f"[TLV ERROR] Heartbeat wrong size: {len(payload_bytes)}")
            self.current_payload = None
            return

        try:
            uptime_ms = struct.unpack("<I", payload_bytes[:4])[0]
            seq = struct.unpack("<H", payload_bytes[4:6])[0]
            status = payload_bytes[6]
            self.last_heartbeat_rx = time.time()

            self.current_payload = {
                "uptime_ms": uptime_ms,
                "seq": seq,
                "status": status,
            }
        except struct.error:
            print("[TLV ERROR] Failed to parse Heartbeat payload")
            self.current_payload = None

    def _read_crc(self) -> Optional[Dict[str, Any]]:
        # Validate CRC and return parsed frame
        crc_start = HEADER_SIZE + self.expected_payload_len
        required_bytes = crc_start + CRC_SIZE

        if len(self.buffer) < required_bytes:
            return None

        try:
            # Extract data for CRC: LEN + TYPE + PAYLOAD
            crc_data = self.buffer[OFFSET_LEN:crc_start]

            # Received CRC
            received_crc = struct.unpack("<H", self.buffer[crc_start:required_bytes])[0]
        except struct.error:
            print("[TLV ERROR] Failed to extract CRC fields")
            self._reset_parser()
            return None

        # Calculate CRC
        calculated_crc = crc16_ccitt(bytes(crc_data))

        # CRC covers: LEN(2) + TYPE(1) + PAYLOAD(n) bytes
        if calculated_crc != received_crc:
            print(
                f"[TLV ERROR] CRC failed: calc {calculated_crc:04X}, recv {received_crc:04X}"
            )
            self.crc_fail += 1
            # CRC fail: skip 1 byte and resync
            self.buffer.pop(0)
            self.state = "SEEKING_SOF"
            return None

        # CRC OK - build frame
        frame = {
            "type": self.current_type,
            "payload": self.current_payload,
            "raw_bytes": bytes(self.buffer[:required_bytes]),
        }

        self.frames_ok += 1

        # Remove processed frame
        self.buffer = self.buffer[required_bytes:]
        self.state = "SEEKING_SOF"

        return frame

    def _reset_parser(self) -> None:
        # Reset parser state (on error)
        self.buffer.clear()
        self.state = "SEEKING_SOF"
        self.expected_payload_len = 0
        self.current_type = 0
        self.current_payload = None
        self.resyncs += 1

    # ================== CONNECTION MONITORING ==================

    def is_connection_healthy(self) -> tuple[bool, float]:
        time_since = time.time() - self.last_heartbeat_rx
        return (time_since <= HEARTBEAT_TIMEOUT, time_since)

    def get_stats(self) -> Dict[str, Any]:
        is_healthy, time_since = self.is_connection_healthy()
        return {
            "frames_ok": self.frames_ok,
            "crc_fail": self.crc_fail,
            "resyncs": self.resyncs,
            "connection_healthy": is_healthy,
            "time_since_heartbeat": time_since,
        }
