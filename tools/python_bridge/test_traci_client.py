import unittest
from unittest.mock import patch
from traci_client import TraCIClient  # adjust path as needed
from traci import TraCIException


class TestTraCIClient(unittest.TestCase):
    @patch("traci_client.traci.simulation.getTime")
    def test_get_time_returns_simulation_time(self, mock_get_time):
        # Arrange
        mock_get_time.return_value = 42.0
        client = TraCIClient(junction_type="x", sumo_cmd=["dummy"])
        client.traci = __import__("traci")  # Assign the real traci module

        # Act
        result = client.get_time()

        # Assert
        self.assertEqual(result, 42.0)

    @patch("traci_client.traci.lane.getLastStepVehicleNumber")
    @patch("traci_client.traci.simulationStep")
    def test_get_lane_vehicle_counts_returns_correct_data(
        self, mock_sim_step, mock_get_count
    ):
        # Arrange
        mock_get_count.side_effect = [3, 1]  # For n1to2, n3to2

        client = TraCIClient(junction_type="x", sumo_cmd=["dummy"])
        client.traci = __import__("traci")  # Assign the real traci module
        client.lanes = ["n1to2", "n3to2"]

        # Act
        result = client.get_lane_vehicle_counts()

        # Assert
        assert result == {"n1to2": 3, "n3to2": 1}

    @patch("traci_client.traci.lane.getLastStepVehicleNumber")
    @patch("traci_client.traci.simulationStep")
    def test_get_lane_vehicle_counts_skips_invalid_lanes(
        self, mock_sim_step, mock_get_count
    ):

        # Arrange
        mock_get_count.side_effect = [TraCIException("Lane not found"), 1]

        client = TraCIClient(junction_type="x", sumo_cmd=["dummy"])
        client.traci = __import__("traci")
        client.lanes = ["n1to2", "n3to2"]

        # Act
        result = client.get_lane_vehicle_counts()

        # Assert
        assert result == {"n3to2": 1}


if __name__ == "__main__":
    unittest.main()
