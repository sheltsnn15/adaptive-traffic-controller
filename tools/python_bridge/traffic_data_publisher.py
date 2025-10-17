import time


class TrafficDataPublisher:
    def __init__(self, traci_client, data_sender):
        self.traci = traci_client
        self.sender = data_sender

    def run_once(self):
        # Pull data from SUMO, format, send
        try:
            # Step 1: Fetch lane vehicle counts
            lane_counts = self.traci.get_lane_vehicle_counts()
            # Step 2: Fetch current simulation time (float seconds)
            timestamp = self.traci.get_time()
            # Step 3: Build the payload
            payload = {
                "lanes": lane_counts,
                "timestamp": int(timestamp),  # optionally round to int
            }
            # Step 4: Publish to MQTT
            self.sender.send(payload)
        except Exception as e:
            print(f"[ERROR] Failed to publish traffic data: {e}")

    def run_forever(self, delay=0.5):
        while True:
            self.run_once()
            time.sleep(delay)
