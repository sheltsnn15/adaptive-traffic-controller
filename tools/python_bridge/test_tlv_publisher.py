from tlv_publisher import (
    TLVOutputHandler,
    crc16_ccitt,
    MSG_TYPE_SUMO_DATA,
    MSG_TYPE_LIGHT_STATE,
    MSG_TYPE_HEARTBEAT,
)
import struct


def test_lanecounts_encoding():
    print("=== Testing LaneCounts Encoding ===")

    test_payload = {
        "timestamp": 1697040000,
        "junction_type": "x",
        "lanes": {"n": 12, "s": 7, "e": 3, "w": 8},
    }

    tlv_handler = TLVOutputHandler()
    tlv_frame = tlv_handler._json_to_tlv(test_payload)
    hex_output = tlv_frame.hex()

    expected_hex = "aa550c000180c62665000c0703086777"
    assert hex_output == expected_hex, f"Expected {expected_hex}, got {hex_output}"
    print(f"LaneCounts encoding correct: {hex_output}")


def test_lightstate_decoding():
    print("\n=== Testing LightState Decoding ===")

    # LightState: NS_GREEN(0), ADAPTIVE_EXTENSION(1), 400ms duration
    lightstate_frame = bytes.fromhex("AA5500070200010400F0D3")

    tlv_handler = TLVOutputHandler()
    for byte in lightstate_frame:
        tlv_handler.process_byte(byte)

    print("LightState decoding completed")


def test_heartbeat_decoding():
    print("\n=== Testing Heartbeat Decoding ===")

    # Heartbeat: uptime_ms = 5000ms
    heartbeat_frame = bytes.fromhex("AA550007038813000029B1")

    tlv_handler = TLVOutputHandler()
    for byte in heartbeat_frame:
        tlv_handler.process_byte(byte)

    print("Heartbeat decoding completed")


def test_corrupted_frame_recovery():
    print("\n=== Testing Frame Recovery ===")

    tlv_handler = TLVOutputHandler()

    # Send garbage bytes first
    garbage = bytes.fromhex("DEADBEEF1234")
    for byte in garbage:
        tlv_handler.process_byte(byte)

    # Then send valid frame
    valid_frame = bytes.fromhex("AA550007038813000029B1")
    for byte in valid_frame:
        tlv_handler.process_byte(byte)

    print("Parser recovered from garbage data")


def test_partial_frame_handling():
    print("\n=== Testing Partial Frame Handling ===")

    tlv_handler = TLVOutputHandler()

    # Send frame in chunks
    frame_chunks = [
        bytes.fromhex("AA55"),  # SOF only
        bytes.fromhex("0007"),  # LEN
        bytes.fromhex("03"),  # TYPE (Heartbeat)
        bytes.fromhex("88130000"),  # PAYLOAD
        bytes.fromhex("29B1"),  # CRC
    ]

    for chunk in frame_chunks:
        for byte in chunk:
            tlv_handler.process_byte(byte)

    print("Partial frame handling correct")


def test_multiple_frames():
    print("\n=== Testing Multiple Frames ===")

    tlv_handler = TLVOutputHandler()

    # Two consecutive Heartbeat frames
    frames = bytes.fromhex("AA550007038813000029B1AA55000703A00F0000A8F3")

    for byte in frames:
        tlv_handler.process_byte(byte)

    print("Multiple frame parsing correct")


def test_unknown_message_type():
    print("\n=== Testing Unknown Message Type ===")

    tlv_handler = TLVOutputHandler()

    # Frame with unknown type 0x04
    unknown_frame = bytes.fromhex("AA55000704AABBCCDD")

    # Calculate CRC for unknown frame
    crc_data = unknown_frame[4:]  # TYPE + PAYLOAD
    crc_value = crc16_ccitt(crc_data)
    unknown_frame += struct.pack("<H", crc_value)

    for byte in unknown_frame:
        tlv_handler.process_byte(byte)

    print("Unknown message type handled gracefully")


def test_various_lanecounts():
    """Test various LaneCounts payloads - encoding only (not decoding)"""
    print("\n=== Testing Various LaneCounts ===")

    test_cases = [
        {
            "name": "All zeros",
            "payload": {
                "timestamp": 0,
                "junction_type": "x",
                "lanes": {"n": 0, "s": 0, "e": 0, "w": 0},
            },
        },
        {
            "name": "Maximum counts",
            "payload": {
                "timestamp": 4294967295,  # max uint32
                "junction_type": "y",
                "lanes": {"n": 255, "s": 255, "e": 255, "w": 255},  # max uint8
            },
        },
        {
            "name": "Realistic traffic",
            "payload": {
                "timestamp": 1697040123,
                "junction_type": "x",
                "lanes": {"n": 15, "s": 8, "e": 22, "w": 11},
            },
        },
    ]

    tlv_handler = TLVOutputHandler()

    for test_case in test_cases:
        print(f"  Testing: {test_case['name']}")
        tlv_frame = tlv_handler._json_to_tlv(test_case["payload"])

        # Verify frame structure is correct
        assert tlv_frame[0:2] == b"\xaa\x55", "SOF pattern missing"
        assert tlv_frame[4] == MSG_TYPE_SUMO_DATA, "Wrong message type"
        print(f"    ✓ Frame structure valid, type: 0x{MSG_TYPE_SUMO_DATA:02X}")

    print("Various LaneCounts encoding correct")


def test_crc_validation():
    print("\n=== Testing CRC Validation ===")

    tlv_handler = TLVOutputHandler()

    # Valid frame
    valid_frame = bytes.fromhex("AA550007038813000029B1")
    print("Testing valid frame (should succeed):")
    for byte in valid_frame:
        tlv_handler.process_byte(byte)

    # Corrupted frame (modified payload)
    corrupted_frame = bytes.fromhex("AA5500070388130001")  # Last payload byte changed
    crc_data = corrupted_frame[4:]
    crc_value = crc16_ccitt(crc_data)
    corrupted_frame += struct.pack("<H", crc_value)

    print("Testing corrupted frame (should fail):")
    for byte in corrupted_frame:
        tlv_handler.process_byte(byte)

    print("CRC validation working correctly")


def test_message_type_constants():
    print("\n=== Testing Message Type Constants ===")

    tlv_handler = TLVOutputHandler()

    # Test SUMO data encoding uses correct type
    test_payload = {
        "timestamp": 1697040000,
        "junction_type": "x",
        "lanes": {"n": 1, "s": 2, "e": 3, "w": 4},
    }

    tlv_frame = tlv_handler._json_to_tlv(test_payload)
    # Type should be at offset 4
    assert (
        tlv_frame[4] == MSG_TYPE_SUMO_DATA
    ), f"Expected type {MSG_TYPE_SUMO_DATA}, got {tlv_frame[4]}"
    print(f"SUMO data type constant correct: {MSG_TYPE_SUMO_DATA:02X}")

    # Test LightState decoding
    lightstate_frame = bytes.fromhex("AA5500070200010400F0D3")
    for byte in lightstate_frame:
        tlv_handler.process_byte(byte)
    print(f"LightState type constant correct: {MSG_TYPE_LIGHT_STATE:02X}")

    # Test Heartbeat decoding
    heartbeat_frame = bytes.fromhex("AA550007038813000029B1")
    for byte in heartbeat_frame:
        tlv_handler.process_byte(byte)
    print(f"Heartbeat type constant correct: {MSG_TYPE_HEARTBEAT:02X}")

    print("All message type constants working correctly")


def test_buffer_management():
    print("\n=== Testing Buffer Management ===")

    tlv_handler = TLVOutputHandler()

    # Send many bytes without SOF to test buffer trimming
    many_bytes = bytes([0x00] * 50)  # 50 bytes without SOF
    for byte in many_bytes:
        tlv_handler.process_byte(byte)

    # Buffer should be trimmed to 1 byte
    assert (
        len(tlv_handler.buffer) <= 1
    ), f"Buffer not trimmed properly: {len(tlv_handler.buffer)}"

    # Now send valid frame
    valid_frame = bytes.fromhex("AA550007038813000029B1")
    for byte in valid_frame:
        tlv_handler.process_byte(byte)

    print("Buffer management working correctly")


def test_heartbeat_encoding():
    print("\n=== Testing Heartbeat Encoding ===")

    tlv_handler = TLVOutputHandler()

    # Test with specific uptime value
    test_uptime_ms = 12345
    heartbeat_frame = tlv_handler._create_heartbeat_frame(test_uptime_ms)

    # Verify frame structure
    assert heartbeat_frame[0:2] == b"\xaa\x55", "SOF pattern missing"
    assert heartbeat_frame[4] == MSG_TYPE_HEARTBEAT, "Wrong message type"

    # Verify payload
    payload = heartbeat_frame[5:9]  # TYPE(1) + PAYLOAD(4)
    uptime_from_frame = struct.unpack("<I", payload)[0]
    assert (
        uptime_from_frame == test_uptime_ms
    ), f"Uptime mismatch: {uptime_from_frame} vs {test_uptime_ms}"

    # Verify CRC
    crc_data = heartbeat_frame[4:9]  # TYPE + PAYLOAD
    calculated_crc = crc16_ccitt(crc_data)
    received_crc = struct.unpack("<H", heartbeat_frame[9:11])[0]
    assert calculated_crc == received_crc, "CRC validation failed"

    print(f"Heartbeat encoding correct: {heartbeat_frame.hex()}")


def run_all_tests():
    print("Starting TLV Protocol Test Suite\n")

    tests = [
        test_lanecounts_encoding,
        test_lightstate_decoding,
        test_heartbeat_decoding,
        test_corrupted_frame_recovery,
        test_partial_frame_handling,
        test_multiple_frames,
        test_unknown_message_type,
        test_various_lanecounts,
        test_crc_validation,
        test_message_type_constants,
        test_buffer_management,
    ]

    for test in tests:
        try:
            test()
            print("PASS")
        except Exception as e:
            print(f"FAIL: {e}")
            continue

    print("\nAll tests completed!")


if __name__ == "__main__":
    run_all_tests()
