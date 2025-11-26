// Core/Src/tlv_parser.cpp
#include "tlv_parser.hpp"
#include "crc16.hpp"
#include <cstdint>
#include <stdint.h>
#include <string.h>

TlvParser::TlvParser()
    : state_(ParseState::IDLE), pos_(0), expected_len_(0),
      crc_calculated_(0xFFFF), crc_bytes_received_(0) {}

uint8_t TlvParser::parseByte(uint8_t byte) {
    switch (state_) {
    case ParseState::IDLE:
        if (byte == 0xAA) {
            state_ = ParseState::HEADER;
            pos_ = 0;
            crc_calculated_ = 0xFFFF;
            buffer_[pos_++] = byte;
        }
        break;
    case ParseState::HEADER:
        buffer_[pos_++] = byte;

        // Start CRC from TYPE byte (position 4) onwards
        if (pos_ == 4) {
            crc_calculated_ = 0xFFFF;
            crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        } else if (pos_ > 4) {
            crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        }

        // Validate SOF byte 2
        if (pos_ == 2 && byte != 0x55) {
            reset();
            break;
        }

        // Complete header received - determine payload length
        if (pos_ == 5) {
            uint8_t msg_type = buffer_[4];

            // expected payload lengths
            switch (msg_type) {
            case 0x01:
                expected_len_ = 9;
                break; // LaneCounts
            case 0x02:
                expected_len_ = 4;
                break; // LightState
            case 0x03:
                expected_len_ = 4;
                break; // Heartbeat
            default:
                reset();
                return 0;
            }

            state_ = ParseState::PAYLOAD;
        }
        break;
    case ParseState::PAYLOAD:
        buffer_[pos_++] = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        if ((pos_ - 5) >= expected_len_) {
            state_ = ParseState::CRC;
            crc_bytes_received_ = 0;
        }
        break;

    case ParseState::CRC:
        buffer_[pos_++] = byte;
        crc_bytes_received_++;

        if (crc_bytes_received_ == 2) {
            // Complete frame received
            if (validateFrame()) {
                uint8_t msg_type = buffer_[4]; // TYPE byte
                dispatchMessage();
                reset();
                return msg_type;
            } else {
                reset(); // CRC failed
            }
        }
        break;
    }
    return 0;
}

bool TlvParser::validateFrame() const {
    if (pos_ < 7)
        return false;

    // Verify SOF bytes
    if (buffer_[0] != 0xAA || buffer_[1] != 0x55) {
        printf("[TLV] ERROR: Invalid SOF bytes\n");
        return false;
    }

    // Verify we only receive expected message types
    uint8_t msg_type = buffer_[4];
    if (msg_type != 0x01 && msg_type != 0x03) { // Only LaneCounts and Heartbeat
        printf("[TLV] ERROR: Unexpected message type: 0x%02X\n", msg_type);
        return false;
    }

    uint16_t received_crc = (buffer_[pos_ - 2] << 8) | buffer_[pos_ - 1];
    bool crc_ok = (received_crc == crc_calculated_);

    if (!crc_ok) {
        printf("[TLV] ERROR: CRC mismatch: calc=0x%04X, recv=0x%04X\n",
               crc_calculated_, received_crc);
    }

    return crc_ok;
}

void TlvParser::dispatchMessage() {
    uint8_t msg_type = buffer_[4];

    switch (msg_type) {
    case 0x01:            // LaneCounts
        if (pos_ >= 16) { // SOF2 + LEN2 + TYPE1 + PAYLOAD9 + CRC2 = 16 bytes
            // Parse little-endian data manually for portability
            lane_counts_.ts_sec =
                (uint32_t)buffer_[5] | ((uint32_t)buffer_[6] << 8) |
                ((uint32_t)buffer_[7] << 16) | ((uint32_t)buffer_[8] << 24);

            lane_counts_.junction_type = buffer_[9];
            lane_counts_.n = buffer_[10];
            lane_counts_.s = buffer_[11];
            lane_counts_.e = buffer_[12];
            lane_counts_.w = buffer_[13];

            // Optional: Debug output
            // printf("LaneCounts: ts=%lu, n=%d, s=%d, e=%d, w=%d\n",
            //        lane_counts_.ts_sec, lane_counts_.n, lane_counts_.s,
            //        lane_counts_.e, lane_counts_.w);
        }
        break;

    case 0x03:            // Heartbeat
        if (pos_ >= 11) { // SOF2 + LEN2 + TYPE1 + PAYLOAD4 + CRC2 = 11 bytes
            // Parse little-endian Heartbeat data
            uint32_t uptime_ms =
                (uint32_t)buffer_[5] | ((uint32_t)buffer_[6] << 8) |
                ((uint32_t)buffer_[7] << 16) | ((uint32_t)buffer_[8] << 24);

            printf("[TLV] Heartbeat received: uptime=%lums\n", uptime_ms);
            break;

        default:
            printf("[TLV] ERROR: Unknown message type: 0x%02X\n", msg_type);
            break;
        }
    }

    void TlvParser::reset() {
        state_ = ParseState::IDLE;
        pos_ = 0;
        expected_len_ = 0;
        crc_calculated_ = 0xFFFF;
        crc_bytes_received_ = 0;
    }
