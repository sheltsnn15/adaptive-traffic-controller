import threading
import time
from typing import Optional

import serial
from tlv_publisher import TLVOutputHandler


class SerialBridge:
    def __init__(self, port: str = "/dev/ttyUSB0", baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.tlv = TLVOutputHandler()

        self.serial: Optional[serial.Serial] = None
        self.running = False
        self.thread: Optional[threading.Thread] = None

    def start(self) -> bool:
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
            )

            self.running = True
            self.thread = threading.Thread(target=self._run_loop, daemon=True)
            self.thread.start()

            print(f"[Serial] Started on {self.port} at {self.baudrate} baud")
            return True

        except Exception as e:
            print(f"[Serial] Failed to open {self.port}: {e}")
            return False

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        if self.serial and self.serial.is_open:
            self.serial.close()
            self.serial = None

        print("[Serial] Stopped")

    def send_lanecounts(self, payload: dict) -> bool:
        if not self.serial or not self.serial.is_open:
            print("[Serial] Not connected, cannot send")
            return False

        try:
            frame = self.tlv.create_lanecounts_frame(payload)
            self.serial.write(frame)
            self.serial.flush()
            print(f"[Python→STM32] Sent: {frame.hex()}")
            return True
        except Exception as e:
            print(f"[Serial] Send error: {e}")
            return False

    def _run_loop(self) -> None:
        print("[Serial] Receive thread started")

        while self.running and self.serial and self.serial.is_open:
            try:
                # Read available data
                if self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)

                    # Process through TLV parser
                    frames = self.tlv.process_bytes(data)

                    # Handle received frames
                    for frame in frames:
                        if frame["type"] == 0x02:  # LightState
                            print(f"[STM32→Python] LightState: {frame['payload']}")
                        elif frame["type"] == 0x03:  # Heartbeat
                            print(f"[STM32→Python] Heartbeat: {frame['payload']}")

                time.sleep(0.001)  # Small sleep

            except Exception as e:
                if self.running:  # Only print if we're supposed to be running
                    print(f"[Serial] Receive error: {e}")
                break

        print("[Serial] Receive thread stopped")

    def get_stats(self) -> dict:
        tlv_stats = self.tlv.get_stats()
        return {
            "connected": self.serial and self.serial.is_open,
            "port": self.port,
            "baudrate": self.baudrate,
            **tlv_stats,
        }

    def is_connected(self) -> bool:
        # Check if serial port is connected and STM32 is sending heartbeats
        serial_connected = self.serial is not None and self.serial.is_open
        tlv_healthy, _ = self.tlv.is_connection_healthy()
        return serial_connected and tlv_healthy
