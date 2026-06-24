from ..tlv_publisher import TLVOutputHandler, crc16_ccitt
import struct


def _create_test_heartbeat_frame() -> bytes:
    """Helper to create a Heartbeat frame from STM32 for testing"""
    frame = bytearray()
    frame.extend(b"\xaa\x55")  # SOF

    # LEN: TYPE(1) + PAYLOAD(4) + CRC(2) = 7
    frame.extend(struct.pack("<H", 7))

    frame.append(0x03)  # TYPE = Heartbeat

    # PAYLOAD: uptime in milliseconds (simulate STM32 uptime)
    uptime_ms = 12345  # Example uptime
    frame.extend(struct.pack("<I", uptime_ms))

    # CRC over LEN + TYPE + PAYLOAD
    crc_data = frame[2:]  # From LEN onward
    crc_value = crc16_ccitt(bytes(crc_data))
    frame.extend(struct.pack("<H", crc_value))

    return bytes(frame)


def _create_test_lightstate_frame(
    current_state: int, decision_reason: int, phase_duration: int
) -> bytes:
    """Helper to create a LightState frame for testing"""
    frame = bytearray()
    frame.extend(b"\xaa\x55")  # SOF

    # LEN: TYPE(1) + PAYLOAD(4) + CRC(2) = 7
    frame.extend(struct.pack("<H", 7))

    frame.append(0x02)  # TYPE = LightState

    # PAYLOAD
    frame.append(current_state & 0xFF)
    frame.append(decision_reason & 0xFF)
    frame.extend(struct.pack("<H", phase_duration))

    # CRC over LEN + TYPE + PAYLOAD
    crc_data = frame[2:]  # From LEN onward
    crc_value = crc16_ccitt(bytes(crc_data))
    frame.extend(struct.pack("<H", crc_value))

    return bytes(frame)


def test_basic_encoding_decoding():
    print("=== Basic Encoding/Decoding ===")
    tlv = TLVOutputHandler()

    # Test LaneCounts encoding
    payload = {
        "timestamp": 1697040000,
        "junction_type": "x",
        "lanes": {"n": 12, "s": 7, "e": 3, "w": 8},
    }
    frame = tlv.create_lanecounts_frame(payload)
    frame_hex = frame.hex()
    print(f"LaneCounts encoding: {frame_hex}")

    # Verify structure
    assert frame[0:2] == b"\xaa\x55", "Wrong SOF"
    assert frame[4] == 0x01, "Wrong message type"
    print("✓ LaneCounts structure OK")

    # Test frame decoding - Python only receives LightState (0x02) and Heartbeat (0x03)
    # Python SENDS LaneCounts (0x01) but doesn't RECEIVE them!
    tlv2 = TLVOutputHandler()

    # Generate a heartbeat frame (from STM32 -> Python)
    hb_frame = _create_test_heartbeat_frame()

    # Generate a LightState frame (from STM32 -> Python)
    lightstate_frame = _create_test_lightstate_frame(
        current_state=0, decision_reason=0, phase_duration=1024
    )

    # Decode only what Python receives from STM32
    test_frames = [
        ("Heartbeat (STM32->Python)", hb_frame),
        ("LightState (STM32->Python)", lightstate_frame),
    ]

    for name, test_frame in test_frames:
        tlv3 = TLVOutputHandler()
        print(f"  Testing {name}: {test_frame.hex()}")
        for byte in test_frame:
            tlv3.process_byte(byte)
        stats = tlv3.get_stats()
        print(f"    Stats: {stats}")
        assert stats["frames_ok"] == 1, f"Failed to decode {name}"
    print("✓ All RECEIVED frame types decode")


def test_error_recovery():
    print("=== Error Recovery ===")
    tlv = TLVOutputHandler()

    # Simpler test: Just test that CRC failure is detected
    valid_frame = _create_test_lightstate_frame(0, 0, 1000)

    # Create bad CRC frame
    bad_frame = bytearray(valid_frame)
    bad_frame[-1] ^= 0xFF  # Flip CRC

    # Test 1: Bad CRC should be rejected
    tlv1 = TLVOutputHandler()
    for byte in bad_frame:
        tlv1.process_byte(byte)
    stats1 = tlv1.get_stats()
    assert stats1["frames_ok"] == 0, "Should reject bad CRC"
    assert stats1["crc_fail"] == 1, "Should count CRC failure"
    print(f"✓ Bad CRC rejected: {stats1}")

    # Test 2: Valid frame after reset should work
    tlv2 = TLVOutputHandler()
    for byte in valid_frame:
        tlv2.process_byte(byte)
    stats2 = tlv2.get_stats()
    assert stats2["frames_ok"] == 1, "Should accept valid frame"
    print(f"✓ Valid frame accepted: {stats2}")

    print("✓ Error recovery working")


def test_real_world_scenarios():
    print("=== Real-World Scenarios ===")

    # Scenario 1: Rapid fire frames (Python receiving LightState frames from STM32)
    tlv = TLVOutputHandler()
    frame = _create_test_lightstate_frame(0, 0, 500)
    rapid_data = frame * 3
    for byte in rapid_data:
        tlv.process_byte(byte)
    assert tlv.frames_ok == 3
    print("✓ Rapid fire frames")

    # Scenario 2: Mixed traffic with glitches
    tlv = TLVOutputHandler()

    # Create test frames that Python would receive from STM32
    ls_frame1 = _create_test_lightstate_frame(1, 0, 512)
    ls_frame2 = _create_test_lightstate_frame(2, 1, 256)
    hb_frame = _create_test_heartbeat_frame()

    mixed_data = (
        b"\x00\x00\xff\xff"  # Startup garbage
        + ls_frame1
        + b"\x00\xaa"  # Partial SOF (garbage)
        + hb_frame  # Heartbeat from STM32
        + ls_frame2  # Another LightState
    )

    for byte in mixed_data:
        tlv.process_byte(byte)

    # Should get 3 frames: ls_frame1, hb_frame, ls_frame2
    assert tlv.frames_ok == 3
    print(f"✓ Mixed traffic with glitches ({tlv.frames_ok} frames)")


def test_edge_cases():
    print("=== Edge Cases ===")

    # Unknown message type - Python should discard it
    tlv = TLVOutputHandler()

    # Create frame with unknown type (0x04)
    frame = bytearray()
    frame.extend(b"\xaa\x55")  # SOF
    frame.extend(struct.pack("<H", 7))  # LEN: TYPE1 + PAYLOAD4 + CRC2
    frame.append(0x04)  # Unknown TYPE
    frame.extend(b"\xaa\xbb\xcc\xdd")  # Random payload
    crc_data = frame[2:]  # LEN onward
    crc_value = crc16_ccitt(bytes(crc_data))
    frame.extend(struct.pack("<H", crc_value))

    for byte in frame:
        tlv.process_byte(byte)

    stats = tlv.get_stats()
    print(f"  Stats after unknown type: {stats}")

    # Should discard unknown message types
    assert stats["frames_ok"] == 0, "Should discard unknown message types"
    print("✓ Unknown message type correctly discarded")

    # Buffer management (50 bytes without SOF)
    tlv = TLVOutputHandler()
    for byte in b"\x00" * 50:
        tlv.process_byte(byte)
    assert len(tlv.buffer) <= 1
    print("✓ Buffer management")

    # Test connection monitoring (heartbeat timeout)
    tlv = TLVOutputHandler()
    # Simulate receiving a heartbeat
    hb_frame = _create_test_heartbeat_frame()
    for byte in hb_frame:
        tlv.process_byte(byte)

    # Check connection is healthy immediately after heartbeat
    healthy, time_since = tlv.is_connection_healthy()
    assert healthy, "Should be healthy right after heartbeat"
    print(f"✓ Connection healthy after heartbeat (time since: {time_since:.2f}s)")


def test_connection_monitoring():
    print("=== Connection Monitoring ===")

    tlv = TLVOutputHandler()

    # Initially, connection should be healthy (no timeout yet)
    healthy, time_since = tlv.is_connection_healthy()
    assert healthy, "Should be healthy initially"
    print(f"  Initial: healthy={healthy}, time_since={time_since:.2f}s")

    # Receive a heartbeat
    hb_frame = _create_test_heartbeat_frame()
    for byte in hb_frame:
        tlv.process_byte(byte)

    # Should be healthy
    healthy, time_since = tlv.is_connection_healthy()
    assert healthy, "Should be healthy after heartbeat"
    print(f"  After heartbeat: healthy={healthy}, time_since={time_since:.2f}s")

    # Note: We can't easily test timeout without mocking time
    print("✓ Connection monitoring basic test passed")


def run_all_tests():
    """Run all tests"""
    print("Starting TLV Test Suite\n")
    print("=" * 60)

    tests = [
        test_basic_encoding_decoding,
        test_error_recovery,
        test_real_world_scenarios,
        test_edge_cases,
        test_connection_monitoring,
    ]

    all_passed = True
    for test in tests:
        try:
            test()
            print("PASS\n")
        except Exception as e:
            print(f"FAIL: {e}\n")
            import traceback

            traceback.print_exc()
            print()
            all_passed = False

    print("=" * 60)
    if all_passed:
        print("✓ All tests passed!")
    else:
        print("✗ Some tests failed")

    return all_passed


if __name__ == "__main__":
    # First, let's generate a test frame to see what hex we get
    print("First, let's see what frames our code generates...\n")

    tlv = TLVOutputHandler()

    # Generate a LaneCounts frame (Python -> STM32)
    payload = {
        "timestamp": 1697040000,
        "junction_type": "x",
        "lanes": {"n": 12, "s": 7, "e": 3, "w": 8},
    }
    frame = tlv.create_lanecounts_frame(payload)
    print(f"LaneCounts frame (Python -> STM32): {frame.hex()}")
    print(f"  SOF: {frame[0:2].hex()}")
    print(f"  LEN: {frame[2:4].hex()} = {struct.unpack('<H', frame[2:4])[0]}")
    print(f"  TYPE: {frame[4]:02x}")
    print(f"  CRC: {frame[-2:].hex()}")

    # Generate test frames for receiving
    print(
        f"\nLightState test frame (STM32 -> Python): {_create_test_lightstate_frame(0, 0, 1024).hex()}"
    )
    print(
        f"Heartbeat test frame (STM32 -> Python): {_create_test_heartbeat_frame().hex()}"
    )

    print("\n" + "=" * 60 + "\n")

    # Run the tests
    run_all_tests()
