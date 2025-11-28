import time
from traffic_data_publisher import TrafficDataPublisher
from traci_client import TraCIClient

# from mqtt_publisher import MQTTPublisher
from tlv_publisher import TLVOutputHandler


# --- Config ---
JUNCTION_TYPE = "x"  # or "y"
# DEVICE_ID = "esp32-001"
# BROKER = "localhost"
# PORT = 1883

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
tlv_publisher = TLVOutputHandler()
traffic_data_publisher = TrafficDataPublisher(traci_client, tlv_publisher)


# --- Run ---
def main():
    try:
        traci_client.start()

        last_heartbeat = time.time()
        heartbeat_interval = 2.0  # Send heartbeat every 2 seconds

        while True:
            # Send traffic data
            traffic_data_publisher.run_once()

            # Send heartbeat if it's time
            current_time = time.time()
            if current_time - last_heartbeat >= heartbeat_interval:
                tlv_publisher.send_heartbeat()
                last_heartbeat = current_time

            time.sleep(0.1)  # Shorter sleep for more responsive heartbeat

    except KeyboardInterrupt:
        print("[EXIT] Interrupted by user.")
    finally:
        traci_client.close()
        print("[EXIT] Cleaned up and disconnected.")


if __name__ == "__main__":
    main()
