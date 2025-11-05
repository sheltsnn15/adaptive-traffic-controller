import traci


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
        self.traci = None

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
        return data

    def get_traffic_light_state(self, tls_id):
        try:
            state = self.traci.trafficlight.getRedYellowGreenState(tls_id)
            phase = self.traci.trafficlight.getPhase(tls_id)
            return {"state":state, "phase": phase}
        except Exception as e:
            print(f"[WARN] could not get TL state: {e}")
            return {"state":"unknown", "phase": -1}


    def get_time(self):
        return self.traci.simulation.getTime()

    def close(self):
        if self.traci:
            self.traci.close()
