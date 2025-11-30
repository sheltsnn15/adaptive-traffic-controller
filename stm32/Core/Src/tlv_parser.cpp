// Core/Src/tlv_parser.cpp
#include "tlv_parser.hpp"
#include "crc16.hpp"
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <string.h>

TlvParser::TlvParser()
    : state_(ParseState::IDLE), current_msg_type_(0), expected_payload_len_(0),
      crc_calculated_(0xFFFF), received_crc_(0), payload_bytes_received_(0),
      crc_bytes_received_(0), timestamp_accumulator_(0) {}

uint8_t TlvParser::parseByte(uint8_t byte) {
    switch (state_) {
    case ParseState::IDLE:
        if (byte == 0xAA) {
            state_ = ParseState::SOF1;
            crc_calculated_ = 0xFFFF;
        }
        break;

    case ParseState::SOF1:
        if (byte == 0x55) {
            state_ = ParseState::LEN1;
        } else {
            reset();
        }
        break;

    case ParseState::LEN1:
        expected_payload_len_ = byte << 8;
        state_ = ParseState::LEN2;
        break;

    case ParseState::LEN2:
        expected_payload_len_ |= byte;
        state_ = ParseState::TYPE;
        break;

    case ParseState::TYPE:
        current_msg_type_ = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);

        if (current_msg_type_ != 0x01 && current_msg_type_ != 0x03) {
            reset();
            break;
        }

        payload_bytes_received_ = 0;
        timestamp_accumulator_ = 0;
        state_ = ParseState::PAYLOAD;
        break;

    case ParseState::PAYLOAD:
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        // Parse based on message type and byte position
        switch (current_msg_type_) {
        case 0x01: // LaneCounts
            parseLaneCountsByte(byte);
            break;
        case 0x03: // Heartbeat
            parseHeartbeatByte(byte);
            break;
        }

        payload_bytes_received_++;

        // Check if payload complete
        if (payload_bytes_received_ >= expected_payload_len_) {
            state_ = ParseState::CRC1;
            crc_bytes_received_ = 0;
        }
        break;

    case ParseState::CRC1:
        received_crc_ = byte << 8;
        state_ = ParseState::CRC2;
        crc_bytes_received_++;
        break;

    case ParseState::CRC2:
        received_crc_ |= byte;
        crc_bytes_received_++;

        if (crc_bytes_received_ == 2) {
            // Complete frame received
            if (validateFrame()) {
                uint8_t msg_type = current_msg_type_;
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
    // check CRC and basic state
    bool crc_ok = (received_crc_ == crc_calculated_);

    if (!crc_ok) {
        printf("[TLV] ERROR: CRC mismatch: calc=0x%04X, recv=0x%04X\n",
               crc_calculated_, received_crc_);
    }

    return crc_ok;
}

void TlvParser::parseLaneCountsByte(uint8_t byte) {
    switch (payload_bytes_received_) {
    case 0: // timestamp
        timestamp_accumulator_ = byte;
        break;
    case 1:
        timestamp_accumulator_ |= (uint32_t)byte << 8;
        break;
    case 2:
        timestamp_accumulator_ |= (uint32_t)byte << 16;
        break;
    case 3:
        timestamp_accumulator_ |= (uint32_t)byte << 24;
        lane_counts_.ts_sec = timestamp_accumulator_;
        printf("[TLV] Timestamp: %lu\n", lane_counts_.ts_sec);
        break;
    case 4: // junction_type
        lane_counts_.junction_type = byte;
        break;
    case 5: // traffic data, lanecounts
        lane_counts_.n = byte;
        break;
    case 6:
        lane_counts_.s = byte;
        break;
    case 7:
        lane_counts_.e = byte;
        break;
    case 8:
        lane_counts_.w = byte;
        printf("[TLV] Counts: n=%d, s=%d, e=%d, w=%d\n", lane_counts_.n,
               lane_counts_.s, lane_counts_.e, lane_counts_.w);
        break;
    default:
        printf("[TLV] ERROR: Unexpected LaneCounts byte position %d\n",
               payload_bytes_received_);
        break;
    }
}

void TlvParser::parseHeartbeatByte(uint8_t byte) {
    switch (payload_bytes_received_) {
    case 0:
    case 1:
    case 2:
    case 3:
        heartbeat.uptime_ms = (heartbeat.uptime_ms << 8) | byte;
        printf("[TLV] Heartbeat uptime: %lu\n", heartbeat_.uptime_ms);
        break;
    default:
        printf("[TLV] ERROR: Unexpected Heartbeat position %d\n",
               payload_bytes_received_);
        break;
    }
}

void TlvParser::reset() {
    state_ = ParseState::IDLE;
    current_msg_type_ = 0;
    expected_payload_len_ = 0;
    crc_calculated_ = 0xFFFF;
    received_crc_ = 0;
    payload_bytes_received_ = 0;
    crc_bytes_received_ = 0;
    timestamp_accumulator_ = 0;
}

bool TlvParser::encodeLightState(const Traffic::LightStatePayload &cmd,
                                 uint8_t *buffer, size_t &len) {
    // TODO: Implementation for sending LightState to SUMO
    return true;
}

bool TlvParser::encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len) {
    // TODO: Implementation for sending Heartbeat to SUMO
    return true;
}
