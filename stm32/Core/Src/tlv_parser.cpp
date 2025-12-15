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
      crc_bytes_received_(0), timestamp_accumulator_(0), frames_ok_(0),
      crc_fails_(0), resyncs_(0) {
    memset(&lane_counts_, 0, sizeof(lane_counts_));
}

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
        } else if (byte == 0xAA) {
            // stay in SOF1
            crc_calculated_ = 0xFFFF;
        } else {
            resetForResync();
            // return parseByte(byte);
        }
        break;

    case ParseState::LEN1:
        expected_payload_len_ = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        state_ = ParseState::LEN2;
        break;

    case ParseState::LEN2:
        expected_payload_len_ |= (uint16_t)byte << 8;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        // Validate LEN: TYPE(1) + PAYLOAD + CRC(2)
        if (expected_payload_len_ < 3 || expected_payload_len_ > 64) {
            printf("[TLV] Invalid LEN: %u\n", expected_payload_len_);
            resetForResync();
            return 0;
        }
        state_ = ParseState::TYPE;
        break;

    case ParseState::TYPE:
        current_msg_type_ = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);

        if (current_msg_type_ != 0x01 && expected_payload_len_ != 12) {
            printf("[TLV] Unexpected message type: 0x%02X\n",
                   current_msg_type_);
            resetForResync();
            break;
        }

        payload_bytes_received_ = 0;
        timestamp_accumulator_ = 0;
        memset(&lane_counts_, 0, sizeof(lane_counts_));
        state_ = ParseState::PAYLOAD;
        break;

    case ParseState::PAYLOAD:
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        // Parse based on message type and byte position
        if (current_msg_type_ == 0x01) {
            parseLaneCountsByte(byte);
        }

        payload_bytes_received_++;

        // Check if payload complete
        if (payload_bytes_received_ >= expected_payload_len_ - 3) {
            state_ = ParseState::CRC1;
            crc_bytes_received_ = 0;
        }
        break;

    case ParseState::CRC1:
        received_crc_ = byte;
        state_ = ParseState::CRC2;
        crc_bytes_received_++;
        break;

    case ParseState::CRC2:
        received_crc_ |= (uint16_t)byte << 8;
        crc_bytes_received_++;

        if (crc_bytes_received_ == 2) {
            // Complete frame received
            if (validateFrame()) {
                frames_ok_++;
                uint8_t msg_type = current_msg_type_;
                reset();
                return msg_type;
            } else {
                crc_fails_++;
                resetForResync();
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
        break;
    default:
        printf("[TLV] ERROR: Unexpected LaneCounts byte position %d\n",
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

void TlvParser::resetForResync() {
    resyncs_++;
    reset();
}

bool TlvParser::encodeLightState(const Traffic::LightStatePayload &cmd,
                                 uint8_t *buffer, size_t &len) {
    // STM32 → Python: LightState (0x02)
    // Frame: SOF(2) + LEN(2) + TYPE(1) + PAYLOAD(4) + CRC(2) = 11 bytes total
    if (len < 11) {
        return false;
    }

    // SOF
    buffer[0] = 0xAA;
    buffer[1] = 0x55;
    // LEN: TYPE(1) + PAYLOAD(4) + CRC(2) = 7
    buffer[2] = 7;
    buffer[3] = 0;

    // TYPE
    buffer[4] = 0x02;

    // PAYLOAD
    buffer[5] = cmd.current_state;
    buffer[6] = cmd.decision_reason;
    buffer[7] = cmd.phase_duration & 0xFF;
    buffer[8] = (cmd.phase_duration >> 8) & 0xFF;

    // Calculate CRC over LEN+TYPE+PAYLOAD
    uint16_t crc = 0xFFFF;
    for (size_t i = 2; i <= 8; i++) {
        crc = Crc16::calculate_byte(buffer[i], crc);
    }

    buffer[9] = crc & 0xFF;
    buffer[10] = (crc >> 8) & 0xFF;

    len = 11;
    return true;
}

bool TlvParser::encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len) {

    // STM32 → Python: Heartbeat(0x03)
    // Frame: SOF(2) + LEN(2) + TYPE(1) + PAYLOAD(7) + CRC(2) = 14 bytes total
    if (len < 14) {
        return false;
    }

    // SOF
    buffer[0] = 0xAA;
    buffer[1] = 0x55;
    // LEN: TYPE(1) + PAYLOAD(7) + CRC(2) = 10
    buffer[2] = 10;
    buffer[3] = 0;

    // TYPE
    buffer[4] = 0x03;

    // PAYLOAD
    buffer[5] = hb.uptime_ms & 0xFF;
    buffer[6] = (hb.uptime_ms >> 8) & 0xFF;
    buffer[7] = (hb.uptime_ms >> 16) & 0xFF;
    buffer[8] = (hb.uptime_ms >> 24) & 0xFF;
    buffer[9] = hb.seq & 0xFF;
    buffer[10] = (hb.seq >> 8) & 0xFF;
    buffer[11] = hb.status;

    // Calculate CRC over LEN+TYPE+PAYLOAD
    uint16_t crc = 0xFFFF;
    for (size_t i = 2; i <= 11; i++) {
        crc = Crc16::calculate_byte(buffer[i], crc);
    }

    buffer[12] = crc & 0xFF;
    buffer[13] = (crc >> 8) & 0xFF;

    len = 14;
    return true;
}
