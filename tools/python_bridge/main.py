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
try:
    traci_client.start()
    traffic_data_publisher.run_forever(delay=0.5)

except KeyboardInterrupt:
    print("[EXIT] Interrupted by user.")

finally:
    traci_client.close()
    # mqtt_publisher.disconnect()
    print("[EXIT] Cleaned up and disconnected.")
