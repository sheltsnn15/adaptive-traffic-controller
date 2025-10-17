import unittest
from unittest.mock import MagicMock, patch
from mqtt_publisher import MQTTPublisher
import json

"""
What TODO Here
1) mock the MQTT client (no real broker used)
2) manually simulate a state
3) check behavior
"""


class TestMQTTPublisher(unittest.TestCase):
    @patch("mqtt_publisher.mqtt.Client")
    def test_send_when_connected(self, mock_mqtt_client_class):
        # Arrange
        mock_client_instance = MagicMock()
        mock_mqtt_client_class.return_value = mock_client_instance

        publisher = MQTTPublisher(
            broker="localhost",
            port=1883,
            junction_type="x-junction",
            device_id="esp32-001",
        )

        # Simulate connection
        publisher.on_connect(mock_client_instance, None, None, rc=0)

        test_payload = {"lanes": {"n1to2": 3}, "timestamp": 1234567890}

        # Act
        publisher.send(test_payload)

        # Assert
        expected_topic = "traffic/x-junction/esp32-001/data"
        expected_payload = json.dumps(test_payload)

        mock_client_instance.publish.assert_called_once_with(
            expected_topic, expected_payload, qos=0
        )

    @patch("mqtt_publisher.mqtt.Client")
    def test_send_when_disconnected(self, mock_mqtt_client_class):
        # Arrange
        mock_client_instance = MagicMock()
        mock_mqtt_client_class.return_value = mock_client_instance

        publisher = MQTTPublisher(
            broker="localhost",
            port=1883,
            junction_type="x-junction",
            device_id="esp32-001",
        )

        # Do NOT call on_connect — simulate disconnected state
        publisher.connected = False

        test_payload = {"lanes": {"n1to2": 5}, "timestamp": 1715123456}

        # Act
        publisher.send(test_payload)

        # Assert
        mock_client_instance.publish.assert_not_called()

    @patch("mqtt_publisher.mqtt.Client")
    def test_send_publish_failure(self, mock_mqtt_client_class):
        # Arrange
        mock_client_instance = MagicMock()
        mock_mqtt_client_class.return_value = mock_client_instance

        # Simulate a failed publish return
        mock_publish_result = MagicMock()
        mock_publish_result.rc = 1
        mock_client_instance.publish.return_value = mock_publish_result

        publisher = MQTTPublisher(
            broker="localhost",
            port=1883,
            junction_type="x-junction",
            device_id="esp32-001",
        )

        publisher.on_connect(mock_client_instance, None, None, rc=0)

        test_payload = {"lanes": {"n1to2": 2}, "timestamp": 1715000000}

        # Act
        publisher.send(test_payload)

        # Assert
        mock_client_instance.publish.assert_called_once()

    @patch("mqtt_publisher.mqtt.Client")
    def test_send_invalid_payload(self, mock_mqtt_client_class):
        # Arrange
        mock_client_instance = MagicMock()
        mock_mqtt_client_class.return_value = mock_client_instance

        publisher = MQTTPublisher(
            broker="localhost",
            port=1883,
            junction_type="x-junction",
            device_id="esp32-001",
        )

        publisher.on_connect(mock_client_instance, None, None, rc=0)

        # Payload that can't be serialized to JSON
        invalid_payload = {
            "lanes": {"n1to2": set([1, 2])},  # sets can't be JSON'd
            "timestamp": 1715000000,
        }

        # Act & Assert: should NOT raise, just print/log
        try:
            publisher.send(invalid_payload)
        except Exception:
            assert False, "send() should not raise on bad payload"

        # Publish should NOT be called
        mock_client_instance.publish.assert_not_called()

    @patch("mqtt_publisher.mqtt.Client")
    def test_disconnect_calls_loop_stop_and_disconnect(self, mock_mqtt_client_class):
        mock_client = MagicMock()
        mock_mqtt_client_class.return_value = mock_client

        publisher = MQTTPublisher(
            broker="localhost",
            port=1883,
            junction_type="x-junction",
            device_id="esp32-001",
        )

        publisher.disconnect()

        mock_client.loop_stop.assert_called_once()
        mock_client.disconnect.assert_called_once()


if __name__ == "__main__":
    unittest.main()
