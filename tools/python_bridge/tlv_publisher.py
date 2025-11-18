import crcmod
import struct


def setup_crc16_ccitt():
    crc16 = crcmod.mkCrcFun(0x11021, initCrc=0xFFFF, rev=False, xorOut=0x0000)
    return crc16


# Global CRC function
crc16_ccitt = setup_crc16_ccitt()


class TLVOutputHandler:

    def __init__(self) -> None:
        self.state = "SEEKING_SOF"
        self.buffer = bytearray()
        self.expected_payload_len = 0
        self.current_type = 0
        self.sof_pattern = b"\xaa\x55"

    def process_byte(self, byte):

        # Add the new byte to buffer
        self.buffer.append(byte)

        if self.state == "SEEKING_SOF":
            # Look for 0xAA55 pattern
            self._seek_start_of_frame()
            # If found, transition to READING_HEADER
        elif self.state == "READING_HEADER":
            # Read LEN and TYPE
            self._read_header()
            # Validate bounds, then transition to READING_PAYLOAD
        elif self.state == "READING_PAYLOAD":
            # Read (LEN-3) bytes
            self._read_payload()
            # Then transition to READING_CRC
        elif self.state == "READING_CRC":
            # Validate CRC, emit frame if valid
            self._read_crc()
            # Always return to SEEKING_SOF

    def _seek_start_of_frame(self):
        pos = self.buffer.find(self.sof_pattern)

        if pos != -1:
            print(f"SOF found at pos {pos}")

            # Remove everything before SOF pattern
            self.buffer = self.buffer[pos:]

            # Transition to reading header
            self.state = "READING_HEADER"
            print("Transitioning to READING_HEADER")

        else:
            # Pattern not found in current buffer
            # Keep only the last byte to handle partial patterns
            if len(self.buffer) > 1:
                # If we have at least 2 bytes but no pattern, keep only last byte
                # This handles cases where we might have received 0xAA but not 0x55 yet
                self.buffer = self.buffer[-1:]

    def _read_header(self):
        if len(self.buffer) >= 5:  # SOF(2) + LEN(2) + TYPE(1)
            # Extract LEN (little-endian)
            length_bytes = self.buffer[2:4]
            self.expected_len = length_bytes[0] + (length_bytes[1] << 8)
            # Extract type
            self.current_type = self.buffer[4]
            # Calculate payload length
            self.payload_len = self.expected_len - 3
            # Transition to READING_HEADER
            self.state = "READING_PAYLOAD"
            print("Transitioning to READING_PAYLOAD")

    def _read_payload(self):
        # We need SOF(2) + HEADER(3) + payload_len bytes in buffer
        required_bytes = 5 + self.payload_len  # 5 = SOF(2) + LEN(2) + TYPE(1)

        if len(self.buffer) >= required_bytes:
            # Extract payload bytes [5:5+payload_len]
            payload_bytes = self.buffer[5 : (5 + self.payload_len)]

            # Parse the payload based on message type
            if self.current_type == 0x02: # Light State
                self.handle_light_state(payload_bytes)

            if self.current_type == 0x03: # Heart beat
                self.handle_heartbeat(payload_bytes)

            # Transition to READING_CRC state
            self.state = "READING_CRC"

    def handle_light_state(self,payload_bytes):
        # Parse the current state
        current_state = payload_bytes[0]

        # Parse decision reason
        decision_reason = payload_bytes[1]

        # Parse phase duration
        phase_duration = (payload_bytes[3] << 8 | payload_bytes[2]) # Little-endian

        # Store the parsed data
        self.current_payload = {
            "current_state": current_state,
            "decision_reason": decision_reason,
            "phase_duration": phase_duration, # convert 100
        }


    def handle_heartbeat(self, payload_bytes):
        pass


    def _read_crc(self):
        # Calculate position: after SOF(2) + HEADER(3) + payload_len
        crc_start = 5 + self.payload_len  # 5 = SOF(2) + LEN(2) + TYPE(1)
        required_bytes = crc_start + 2

        if len(self.buffer) >= required_bytes:
            # Extract the data that was covered by CRC (TYPE + PAYLOAD)
            crc_data_start = 4  # TYPE is at position 4
            crc_data_end = crc_start  # End before CRC
            data_for_crc = self.buffer[crc_data_start:crc_data_end]

            # Extract received CRC (little-endian)
            crc_bytes = self.buffer[crc_start : crc_start + 2]
            received_crc = crc_bytes[0] + (crc_bytes[1] << 8)

            # Calculate expected CRC
            calculated_crc = crc16_ccitt(bytes(data_for_crc))

            # Validate
            if calculated_crc == received_crc:
                print("!CRC validated - frame accepted")

                # Complete payload frame and send
                complete_frame = {
                    "type": self.current_type,
                    "payload": self.current_payload,
                    "raw_bytes": self.buffer[:required_bytes],
                }
                self.send(complete_frame)

            else:
                print(
                    f"CRC failed: calculated {calculated_crc:04X}, received {received_crc:04X}"
                )

            # Remove processed frame from buffer
            self.buffer = self.buffer[required_bytes:]

            # Always return to SEEKING_SOF for next frame
            self.state = "SEEKING_SOF"

    def send(self, payload):
        if 'type' in payload and 'payload' in payload:
            # This is a decoded frame from STM32
            print(f"[STM32→PYTHON] Received: {payload}")
        else:
            # This is SUMO data to encode for STM32
            # Convert JSON to TLV binary
            tlv_data = self.json_to_tlv(payload)
            # Send to serial port
            # self.serial.write(tlv_data)
            print(f"[TLV] Would send {len(tlv_data)} bytes")

    def json_to_tlv(self, payload):
        # Extract data
        timestamp = payload["timestamp"]
        junction_type = 0 if payload["junction_type"] == "x" else 1
        lanes = payload["lanes"]

        # Build the payload
        payload_bytes = struct.pack(
            "<LBBBBB",
            timestamp,
            junction_type,
            lanes["n"],
            lanes["s"],
            lanes["e"],
            lanes["w"],
        )

        # 2. Build frame: SOF + LEN + TYPE + PAYLOAD + CRC
        frame = bytearray()

        frame.extend(b"\xaa\x55")
        # LEN = TYPE(1) + PAYLOAD(9) + CRC(2) = 12
        frame.extend(struct.pack("<H", 12))
        frame.extend(b"\x01")
        frame.extend(payload_bytes)
        # CRC (over TYPE + PAYLOAD only)
        crc_data = frame[4:]
        crc_value = crc16_ccitt(bytes(crc_data))
        frame.extend(struct.pack("<H", crc_value))

        return bytes(frame)
