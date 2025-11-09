from tlv_publisher import TLVOutputHandler

# Test with known data
test_payload = {
    "timestamp": 1697040000,
    "junction_type": "x",
    "lanes": {"n": 12, "s": 7, "e": 3, "w": 8},
}

tlv_handler = TLVOutputHandler()
tlv_frame = tlv_handler.json_to_tlv(test_payload)
hex_output = tlv_frame.hex()

expected_hex = "aa550c000180c62665000c0703086777"
assert hex_output == expected_hex, f"Expected {expected_hex}, got {hex_output}"

print("\nTesting decoder...")
for byte in tlv_frame:
    tlv_handler.process_byte(byte)
