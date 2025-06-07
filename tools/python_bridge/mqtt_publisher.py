import paho.mqtt.client as mqtt
import json
import time

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "traffic/"


class MQTTPublisher:
    def __init__(self, broker: str, port: int, junction_type: str, device_id: str):
        self.broker = broker
        self.port = port
        self.topic = f"traffic/{junction_type}/{device_id}/data"
        self.connected = False
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_publish = self.on_publish

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print("[MQTT] Connected to broker.")
        else:
            print(f"[MQTT] Failed to connect. Return code: {rc}")

    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        print("[MQTT] Disconnected from broker.")

    def on_publish(self, client, userdata, mid):
        print(f"[MQTT] Message {mid} published.")

    def send(self, payload: dict) -> None:
        max_retries = 3
        wait_time = 0.5  # seconds

        for attempt in range(max_retries):
            if self.connected:
                break
            print(f"[MQTT] Not connected. Retry {attempt + 1}/{max_retries}...")
            time.sleep(wait_time)
        else:
            print("[MQTT] Failed to connect after retries. Dropping message.")
            return

        try:
            message = json.dumps(payload)
            result = self.client.publish(self.topic, message, qos=0)

            # Optional: check publish success (returns MQTTMessageInfo)
            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                print(f"[MQTT] Publish failed with code: {result.rc}")
        except Exception as e:
            print(f"[MQTT] Exception while publishing: {e}")

    def disconnect(self):
        print("[MQTT] Disconnecting from broker...")
        self.client.loop_stop()
        self.client.disconnect()
