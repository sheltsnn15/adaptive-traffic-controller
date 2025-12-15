import time
import sys
from traffic_data_publisher import TrafficDataPublisher
from traci_client import TraCIClient

# from mqtt_publisher import MQTTPublisher
from serial_bridge import SerialBridge


# --- Config ---
JUNCTION_TYPE = "x"  # or "y"
# DEVICE_ID = "esp32-001"
# BROKER = "localhost"
# PORT = 1883
SERIAL_PORT = "/dev/ttyUSB0"
SERIAL_BAUDRATE = 115200

# Load variables from .env file into environment
# load_dotenv()

# USERNAME = os.getenv("MQTT_USERNAME")
# PASSWORD = os.getenv("MQTT_PASSWORD")


# SUMO command (adjust paths as needed)
SUMO_CMD = [
    "sumo",
    "-c",
    f"../../sim/{JUNCTION_TYPE}_junction/{JUNCTION_TYPE}_junction.sumocfg",
]

# --- Setup ---
traci_client = TraCIClient(JUNCTION_TYPE, SUMO_CMD)
# mqtt_publisher = MQTTPublisher(
#     BROKER, PORT, JUNCTION_TYPE, DEVICE_ID, USERNAME, PASSWORD
# )
# tlv_publisher = TLVOutputHandler()
serial_bridge = SerialBridge(port=SERIAL_PORT, baudrate=SERIAL_BAUDRATE)
traffic_data_publisher = TrafficDataPublisher(traci_client, serial_bridge)


# --- Run ---
def main():
    try:
        print("[SUMO] Starting TraCI connection...")
        traci_client.start()

        # Send traffic data
        traffic_data_publisher.run_once()

        # Start serial connection to STM32
        print(f"[Serial] Connecting to {SERIAL_PORT} at {SERIAL_BAUDRATE} baud...")
        if not serial_bridge.start():
            print(f"[ERROR] Failed to connect to serial port {SERIAL_PORT}")
            print("Possible fixes:")
            print("  1. Check if STM32 is connected")
            print("  2. Check port name (/dev/ttyUSB0, /dev/ttyACM0, COM3, etc.)")
            print("  3. Check if another program is using the port")
            sys.exit(1)

        print("[System] All connections established. Starting main loop...")
        print("[System] Press Ctrl+C to stop.")

        # Main loop variables
        stats_interval = 5.0  # Print stats every 5 seconds
        last_stats_time = time.time()

        while True:
            try:
                # Send traffic data (LaneCounts to STM32)
                traffic_data_publisher.run_once()

                # Check connection health
                if not serial_bridge.is_connected():
                    print("[WARNING] STM32 connection unhealthy! Check heartbeat.")

                # Print statistics periodically
                current_time = time.time()
                if current_time - last_stats_time >= stats_interval:
                    stats = serial_bridge.get_stats()
                    print(f"\n[Stats] {stats}")
                    last_stats_time = current_time

                # Small sleep to prevent CPU spinning
                # TrafficDataPublisher.run_once() already has its timing
                # via TraCI simulation step
                time.sleep(0.01)  # 10ms

            except KeyboardInterrupt:
                print("\n[EXIT] Interrupted by user.")
                break
            except Exception as e:
                print(f"[ERROR] Unexpected error in main loop: {e}")
                # Continue running unless it's a fatal error
                time.sleep(1.0)  # Wait before retrying

    except KeyboardInterrupt:
        print("\n[EXIT] Interrupted during startup.")
    except Exception as e:
        print(f"[ERROR] Fatal error: {e}")
    finally:
        # Cleanup
        print("[EXIT] Cleaning up...")
        serial_bridge.stop()
        traci_client.close()
        print("[EXIT] Cleanup complete.")


if __name__ == "__main__":
    main()
