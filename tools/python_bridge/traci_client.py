import traci


def map_lanes_to_directions(lane_counts, junction_type):
    if junction_type == "x":
        return {
            "n": lane_counts.get("n5to2_0", 0),
            "s": lane_counts.get("n4to2_0", 0),
            "e": lane_counts.get("n1to2_0", 0),
            "w": lane_counts.get("n3to2_0", 0),
        }
    elif junction_type == "y":
        return {
            "n": lane_counts.get("n1toC_0", 0),
            "s": lane_counts.get("n2toC_0", 0),
            "e": lane_counts.get("n3toC_0", 0),
            "w": 0,
        }


class TraCIClient:
    def __init__(self, junction_type, sumo_cmd):
        self.junction_type = junction_type  # e.g. "x", "y"
        # Define which lanes to monitor per junction type
        self.lane_map = {
            "x": ["n1to2_0", "n3to2_0", "n4to2_0", "n5to2_0"],
            "y": ["n1toC_0", "n2toC_0", "n3toC_0"],
        }
        self.lanes = self.lane_map.get(junction_type, [])
        self.sumo_cmd = sumo_cmd

    def start(self):
        traci.start(self.sumo_cmd)
        self.traci = traci

    def get_lane_vehicle_counts(self):
        self.traci.simulationStep()  # advance the simulation
        # Read vehicle counts from each lane
        data = {}
        for lane_id in self.lanes:
            try:
                data[lane_id] = self.traci.lane.getLastStepVehicleNumber(lane_id)
            except self.traci.TraCIException:
                print(f"[WARN] Lane '{lane_id}' not found.")
        mapped_data = map_lanes_to_directions(data, self.junction_type)
        return mapped_data

    def get_time(self):
        return self.traci.simulation.getTime()

    def close(self):
        if self.traci:
            self.traci.close()
