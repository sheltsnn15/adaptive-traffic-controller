// Core/Inc/tlv_parser.hpp
#pragma once
#include "cppcrc.hpp"
#include "traffic_types.hpp"
#include <cstdint>

class TlvParser {
  public:
    enum class ParseState {
        IDLE,
        HEADER,
        PAYLOAD,
        CRC,
    };

    TlvParser();

    // Returns message type if complete frame parsed, 0 otherwise
    uint8_t parseByte(uint8_t byte);

    // Get parsed data (only valid after parseByte returns non-zero)
    const Traffic::LaneCounts &getLaneCounts() const { return lane_counts_; }

    // Reset parser state
    void reset();

    // Encoding methods
    static bool encodeLightState(const Traffic::LightStatePayload &cmd,
                                 uint8_t *buffer, size_t &len);
    static bool encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len);

  private:
    ParseState state_;
    uint8_t buffer_[32];
    uint8_t pos_;
    uint16_t expected_len_;
    uint16_t crc_calculated_;
    uint8_t crc_bytes_received_;
    Traffic::LaneCounts lane_counts_;

    bool validateFrame() const;
    void dispatchMessage();
};
