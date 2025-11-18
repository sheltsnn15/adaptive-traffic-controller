// Core/Inc/tlv_parser.hpp
#pragma once
#include "cppcrc.hpp"
#include "traffic_types.hpp"

class TlvParser {
  public:
    enum class ParseState {
        IDLE,
        SOF1,
        SOF2,
        LEN1,
        LEN2,
        TYPE,
        PAYLOAD,
        CRC1,
        CRC2
    };

    TlvParser();

    // Returns message type if complete frame parsed, 0 otherwise
    uint8_t parseByte(uint8_t byte);

    // Get parsed data (only valid after parseByte returns non-zero)
    const LaneCounts &getLaneCounts() const { return lane_counts_; }

    // Reset parser state
    void reset();

    // Encoding methods
    static bool encodeLightState(const LightStatePayload &cmd, uint8_t *buffer,
                                 size_t &len);
    static bool encodeHeartbeat(const Heartbeat &hb, uint8_t *buffer,
                                size_t &len);

  private:
    ParseState state_;
    uint8_t buffer_[32];
    uint8_t pos_;
    uint16_t expected_len_;
    uint16_t crc_calculated_;
    LaneCounts lane_counts_;

    bool validateFrame() const;
    void dispatchMessage();
};
