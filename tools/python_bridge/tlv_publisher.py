import crcmod
import time
import struct
from typing import Optional, Dict, Any

# Protocol constants
SOF_PATTERN = bytes([0xAA, 0x55])
MAX_FRAME_LEN = 32

# Message types
MSG_TYPE_SUMO_DATA = 0x01
MSG_TYPE_LIGHT_STATE = 0x02
MSG_TYPE_HEARTBEAT = 0x03

# Frame structure offsets
OFFSET_SOF = 0
OFFSET_LEN = 2
OFFSET_TYPE = 4
HEADER_SIZE = 5  # SOF(2) + LEN(2) + TYPE(1)
CRC_SIZE = 2

# Payload sizes
LIGHT_STATE_PAYLOAD_SIZE = 4
HEARTBEAT_PAYLOAD_SIZE = 4
SUMO_DATA_PAYLOAD_SIZE = 9


def setup_crc16_ccitt():
    return crcmod.mkCrcFun(0x11021, initCrc=0xFFFF, rev=False, xorOut=0x0000)


# Global CRC function
crc16_ccitt = setup_crc16_ccitt()


class TLVOutputHandler:

    def __init__(self) -> None:
        self.state = "SEEKING_SOF"
        self.buffer = bytearray()
        self.expected_payload_len = 0
        self.current_type = 0
        self.current_payload: Optional[Dict[str, Any]] = None
        self.start_time = time.time()

    def process_byte(self, byte: bytes | int) -> None:
        # Convert to integer if needed
        if isinstance(byte, bytes):
            if len(byte) != 1:
                raise ValueError("process_byte expects a single byte")
            byte = byte[0]
        elif not isinstance(byte, int) or not (0 <= byte <= 255):
            raise TypeError("process_byte expects int or single-byte bytes")

        # Add the new byte to buffer
        self.buffer.append(byte)

        # State machine
        if self.state == "SEEKING_SOF":
            self._seek_start_of_frame()
        elif self.state == "READING_HEADER":
            self._read_header()
        elif self.state == "READING_PAYLOAD":
            self._read_payload()
        elif self.state == "READING_CRC":
            self._read_crc()

    def _seek_start_of_frame(self) -> None:
        pos = self.buffer.find(SOF_PATTERN)

        if pos != -1:
            # Remove everything before SOF pattern
            self.buffer = self.buffer[pos:]
            self.state = "READING_HEADER"
        else:
            # Pattern not found, keep only last byte to handle partial patterns
            if len(self.buffer) > 1:
                if len(self.buffer) > MAX_FRAME_LEN:
                    # Keep only last byte to prevent runaway memory
                    self.buffer = self.buffer[-1:]
                else:
                    # Keep only last byte for possible partial SOF
                    self.buffer = self.buffer[-1:]

    def _read_header(self) -> None:
        if len(self.buffer) < HEADER_SIZE:
            return

        # Extract LEN (little-endian)
        length_bytes = self.buffer[OFFSET_LEN:OFFSET_TYPE]
        total_frame_len = struct.unpack("<H", length_bytes)[0]

        # Validate length per spec
        # total_frame_len must be at least TYPE(1)+CRC(2)=3, but we expect more
        if total_frame_len < 4 or total_frame_len > MAX_FRAME_LEN:
            # Invalid length - discard first byte and resync
            self.buffer = self.buffer[1:]
            self.state = "SEEKING_SOF"
            return

        # Extract type and calculate payload length
        self.current_type = self.buffer[OFFSET_TYPE]
        self.expected_payload_len = total_frame_len - 3  # TYPE(1) + CRC(2)
        self.state = "READING_PAYLOAD"

    def _read_payload(self) -> None:
        required_bytes = HEADER_SIZE + self.expected_payload_len

        if len(self.buffer) < required_bytes:
            return

        payload_bytes = self.buffer[HEADER_SIZE:required_bytes]

        # Parse based on message type
        if self.current_type == MSG_TYPE_LIGHT_STATE:
            self._handle_light_state(payload_bytes)
        elif self.current_type == MSG_TYPE_HEARTBEAT:
            self._handle_heartbeat(payload_bytes)
        else:
            print(f"Unknown message type: 0x{self.current_type:02X}")

        self.state = "READING_CRC"

    def _handle_light_state(self, payload_bytes: bytes) -> None:
        if len(payload_bytes) < LIGHT_STATE_PAYLOAD_SIZE:
            print("ERROR: LightState payload too short")
            return

        current_state = payload_bytes[0]
        decision_reason = payload_bytes[1]
        phase_duration = struct.unpack("<H", payload_bytes[2:4])[0]

        self.current_payload = {
            "current_state": current_state,
            "decision_reason": decision_reason,
            "phase_duration": phase_duration,
        }

    def _handle_heartbeat(self, payload_bytes: bytes) -> None:
        if len(payload_bytes) < HEARTBEAT_PAYLOAD_SIZE:
            print("ERROR: Heartbeat payload too short")
            return

        # Parse little-endian uint32 uptime_ms
        uptime_ms = struct.unpack("<I", payload_bytes[:4])[0]

        self.current_payload = {
            "uptime_ms": uptime_ms,
        }

    def _read_crc(self) -> None:
        crc_start = HEADER_SIZE + self.expected_payload_len
        required_bytes = crc_start + CRC_SIZE

        if len(self.buffer) < required_bytes:
            return

        # Extract data covered by CRC (TYPE + PAYLOAD)
        data_for_crc = self.buffer[OFFSET_TYPE:crc_start]

        # Extract received CRC (little-endian)
        received_crc = struct.unpack("<H", self.buffer[crc_start:required_bytes])[0]

        # Calculate and validate CRC
        calculated_crc = crc16_ccitt(bytes(data_for_crc))

        if calculated_crc == received_crc:
            complete_frame = {
                "type": self.current_type,
                "payload": self.current_payload,
                "raw_bytes": bytes(self.buffer[:required_bytes]),
            }
            self._send_received(complete_frame)
        else:
            print(f"CRC failed: calc {calculated_crc:04X}, recv {received_crc:04X}")

        # Remove processed frame and reset state
        self.buffer = self.buffer[required_bytes:]
        self.state = "SEEKING_SOF"

    def _send_received(self, payload: Dict[str, Any]) -> None:
        print(f"[STM32→PYTHON] Received: {payload}")

    def send_heartbeat(self) -> None:
        # Calculate how long Python has been running
        uptime_ms = int((time.time() - self.start_time) * 1000)

        # Create the heartbeat frame
        heartbeat_frame = self._create_heartbeat_frame(uptime_ms)

        print(f"[HEARTBEAT] Sending uptime: {uptime_ms}ms")
        # self.serial.write(heartbeat_frame)  # Uncomment when serial is available

    def _create_heartbeat_frame(self, uptime_ms: int) -> bytes:
        # Build frame step by step
        frame = bytearray()

        # 1. Start of Frame (SOF) pattern
        frame.extend(SOF_PATTERN)

        # 2. Length: TYPE(1) + PAYLOAD(4) + CRC(2) = 7 bytes total
        frame.extend(struct.pack("<H", 7))

        # 3. Message Type
        frame.extend(bytes([MSG_TYPE_HEARTBEAT]))  # 0x03

        # 4. Payload: uptime in milliseconds (32-bit little-endian)
        frame.extend(struct.pack("<I", uptime_ms))

        # 5. Calculate CRC over TYPE + PAYLOAD
        crc_data = frame[OFFSET_TYPE:]  # Everything from TYPE onward
        crc_value = crc16_ccitt(bytes(crc_data))
        frame.extend(struct.pack("<H", crc_value))

        return bytes(frame)

    def send(self, payload: Dict[str, Any]) -> None:
        tlv_data = self._json_to_tlv(payload)
        print(f"[TLV] Would send {len(tlv_data)} bytes")

    def _json_to_tlv(self, payload: Dict[str, Any]) -> bytes:
        timestamp = payload["timestamp"]
        junction_type = 0 if payload["junction_type"] == "x" else 1
        lanes = payload["lanes"]

        # Build payload
        payload_bytes = struct.pack(
            "<LBBBBB",
            timestamp,
            junction_type,
            lanes["n"],
            lanes["s"],
            lanes["e"],
            lanes["w"],
        )

        # Build frame
        frame = bytearray()
        frame.extend(SOF_PATTERN)
        frame.extend(struct.pack("<H", SUMO_DATA_PAYLOAD_SIZE + 3))  # TYPE(1) + CRC(2)
        frame.extend(bytes([MSG_TYPE_SUMO_DATA]))
        frame.extend(payload_bytes)

        # Calculate and append CRC
        crc_data = frame[OFFSET_TYPE:]
        crc_value = crc16_ccitt(bytes(crc_data))
        frame.extend(struct.pack("<H", crc_value))

        return bytes(frame)

    def reset(self) -> None:
        self.state = "SEEKING_SOF"
        self.buffer.clear()
        self.expected_payload_len = 0
        self.current_type = 0
        self.current_payload = None
