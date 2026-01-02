// Core/Inc/tlv_parser.hpp
#pragma once
#include "traffic_types.hpp"
#include <cstddef>
#include <cstdint>

class TlvParser { // direct byte-to-struct parsing
  public:
    static constexpr uint8_t kSof0 = 0xAA, kSof1 = 0x55;
    static constexpr uint16_t kCrcInit = 0xFFFF;

    // Protocol building blocks
    static constexpr uint8_t kSofSize = 2, kLenFieldSize = 2;
    static constexpr size_t kHeaderSize = kSofSize + kLenFieldSize; // SOF+LEN

    static constexpr uint8_t kTypeSize = 1, kCrcSize = 2;
    static constexpr uint16_t kMinLen = kTypeSize + kCrcSize, // TYPE + CRC
        kMaxLen = 64;

    struct MsgSpec {
        uint8_t type;
        uint8_t payload_size; // payload bytes only

        constexpr uint16_t len_field() const noexcept {
            return TlvParser::kTypeSize + payload_size + TlvParser::kCrcSize;
        }

        constexpr size_t frame_size() const noexcept {
            return TlvParser::kHeaderSize + len_field();
        }
    };

    static constexpr MsgSpec laneCountsSpec() noexcept { return {0x01, 9}; }
    static constexpr MsgSpec lightStateSpec() noexcept { return {0x02, 4}; }
    static constexpr MsgSpec heartbeatSpec() noexcept { return {0x03, 7}; }

    enum class ParseState {

        IDLE,
        SOF1,
        LEN1,
        LEN2,
        TYPE,
        PAYLOAD,
        CRC1,
        CRC2,
    };

    TlvParser();

    // Returns message type if complete frame parsed, 0 otherwise
    uint8_t parseByte(uint8_t byte);

    // Get parsed data (only valid after parseByte returns non-zero)
    const Traffic::LaneCounts &getLaneCounts() const noexcept {
        return lane_counts_;
    }

    // Reset parser state
    void reset();

    // Statistics
    uint32_t getFramesOk() const noexcept { return frames_ok_; }
    uint32_t getCrcFails() const noexcept { return crc_fails_; }
    uint32_t getResyncs() const noexcept { return resyncs_; }

    // Encoding methods
    static bool encodeLightState(const Traffic::LightStatePayload &cmd,
                                 uint8_t *buffer, size_t &len);
    static bool encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len);

  private:
    ParseState state_{ParseState::IDLE};
    uint8_t current_msg_type_{0}, crc_bytes_received_{0};
    uint16_t frame_len_{0}, payload_bytes_received_{0},
        crc_calculated_{kCrcInit}, received_crc_{0};

    // Statistics
    uint32_t frames_ok_{0}, crc_fails_{0}, resyncs_{0};

    // Current message being parsed directly into structs
    Traffic::LaneCounts lane_counts_{};
    uint32_t timestamp_accumulator_{0}; // For building multi-byte timestamp

    bool validateFrame() const;
    void parseLaneCountsByte(uint8_t byte);
    void resetForResync();
};
