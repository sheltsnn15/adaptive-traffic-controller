// Core/Src/tlv_parser.cpp
#include "tlv_parser.hpp"
#include "crc16.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>

#ifdef TLV_DEBUG
#define TLV_LOG(...) printf(__VA_ARGS__)
#else
#define TLV_LOG(...)                                                           \
    do {                                                                       \
    } while (0)
#endif

TlvParser::TlvParser() = default;

uint8_t TlvParser::parseByte(uint8_t byte) {
    constexpr auto lc_spec = laneCountsSpec();
    uint16_t payload_len = 0;

    switch (state_) {
    case ParseState::IDLE:
        if (byte == kSof0) {
            state_ = ParseState::SOF1;
            crc_calculated_ = kCrcInit;
        }
        break;

    case ParseState::SOF1:
        if (byte == kSof1) {
            state_ = ParseState::LEN1;
        } else if (byte == kSof0) {
            // stay in SOF1
            crc_calculated_ = kCrcInit;
        } else {
            resetForResync();
        }
        break;

    case ParseState::LEN1:
        frame_len_ = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        state_ = ParseState::LEN2;
        break;

    case ParseState::LEN2:
        frame_len_ |= static_cast<uint16_t>(byte) << 8;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        // Validate LEN: TYPE(1) + PAYLOAD + CRC(2)
        if (frame_len_ < kMinLen || frame_len_ > kMaxLen) {
            TLV_LOG("[TLV] Invalid LEN: %u\n", frame_len_);
            resetForResync();
            return 0;
        }
        state_ = ParseState::TYPE;
        break;

    case ParseState::TYPE:
        current_msg_type_ = byte;
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);

        if (current_msg_type_ != lc_spec.type ||
            frame_len_ != lc_spec.len_field()) {
            if (current_msg_type_ != lc_spec.type) {
                TLV_LOG("[TLV] Unexpected message type: 0x%02X\n",
                        current_msg_type_);
            } else {
                TLV_LOG("[TLV] LaneCounts LEN wrong: %u\n", frame_len_);
            }
            resetForResync();
            break;
        }

        payload_bytes_received_ = 0;
        timestamp_accumulator_ = 0;
        lane_counts_ = {};
        state_ = ParseState::PAYLOAD;
        break;

    case ParseState::PAYLOAD:
        crc_calculated_ = Crc16::calculate_byte(byte, crc_calculated_);
        // Parse based on message type and byte position
        if (current_msg_type_ == lc_spec.type) {
            parseLaneCountsByte(byte);
        }

        payload_bytes_received_++;

        // Check if payload complete
        payload_len = frame_len_ - kMinLen; // LEN - (TYPE+CRC)
        if (payload_bytes_received_ >= payload_len) {
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
        received_crc_ |= static_cast<uint16_t>(byte) << 8;
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
        TLV_LOG("[TLV] ERROR: CRC mismatch: calc=0x%04X, recv=0x%04X\n",
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
        timestamp_accumulator_ |= static_cast<uint32_t>(byte) << 8;
        break;
    case 2:
        timestamp_accumulator_ |= static_cast<uint32_t>(byte) << 16;
        break;
    case 3:
        timestamp_accumulator_ |= static_cast<uint32_t>(byte) << 24;
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
        TLV_LOG("[TLV] ERROR: Unexpected LaneCounts byte position %u\n",
                payload_bytes_received_);
        break;
    }
}

void TlvParser::reset() {
    state_ = ParseState::IDLE;
    current_msg_type_ = 0;
    frame_len_ = 0;
    crc_calculated_ = kCrcInit;
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
    // STM32 -> Python: LightState (0x02)
    constexpr auto ls_spec = lightStateSpec();
    if (len < ls_spec.frame_size()) {
        return false;
    }

    // SOF
    buffer[0] = kSof0;
    buffer[1] = kSof1;

    const uint16_t len_field = ls_spec.len_field();
    buffer[2] = static_cast<uint8_t>(len_field & 0xFF);
    buffer[3] = static_cast<uint8_t>((len_field >> 8) & 0xFF);

    // TYPE
    buffer[4] = ls_spec.type;

    // PAYLOAD
    buffer[5] = cmd.current_state;
    buffer[6] = cmd.decision_reason;
    buffer[7] = static_cast<uint8_t>(cmd.phase_duration & 0xFF);
    buffer[8] = static_cast<uint8_t>((cmd.phase_duration >> 8) & 0xFF);

    // CRC over LEN(2) + TYPE(1) + PAYLOAD(N)
    const size_t crc_first = 2;
    const size_t crc_count = kLenFieldSize + kTypeSize + ls_spec.payload_size;
    const size_t crc_last = crc_first + crc_count - 1;

    uint16_t crc = kCrcInit;
    for (size_t i = crc_first; i <= crc_last; ++i) {
        crc = Crc16::calculate_byte(buffer[i], crc);
    }

    buffer[crc_last + 1] = static_cast<uint8_t>(crc & 0xFF);
    buffer[crc_last + 2] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    len = ls_spec.frame_size();
    return true;
}

bool TlvParser::encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len) {

    // STM32 -> Python: Heartbeat(0x03)
    constexpr auto hb_spec = heartbeatSpec();
    if (len < hb_spec.frame_size()) {
        return false;
    }

    // SOF
    buffer[0] = kSof0;
    buffer[1] = kSof1;

    const uint16_t len_field = hb_spec.len_field();
    buffer[2] = static_cast<uint8_t>(len_field & 0xFF);
    buffer[3] = static_cast<uint8_t>((len_field >> 8) & 0xFF);

    buffer[4] = hb_spec.type;

    buffer[5] = static_cast<uint8_t>(hb.uptime_ms & 0xFF);
    buffer[6] = static_cast<uint8_t>((hb.uptime_ms >> 8) & 0xFF);
    buffer[7] = static_cast<uint8_t>((hb.uptime_ms >> 16) & 0xFF);
    buffer[8] = static_cast<uint8_t>((hb.uptime_ms >> 24) & 0xFF);
    buffer[9] = static_cast<uint8_t>(hb.seq & 0xFF);
    buffer[10] = static_cast<uint8_t>((hb.seq >> 8) & 0xFF);
    buffer[11] = hb.status;

    const size_t crc_first = 2;
    const size_t crc_count = kLenFieldSize + kTypeSize + hb_spec.payload_size;
    const size_t crc_last = crc_first + crc_count - 1;

    uint16_t crc = kCrcInit;
    for (size_t i = crc_first; i <= crc_last; ++i) {
        crc = Crc16::calculate_byte(buffer[i], crc);
    }

    buffer[crc_last + 1] = static_cast<uint8_t>(crc & 0xFF);
    buffer[crc_last + 2] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    len = hb_spec.frame_size();
    return true;
}
