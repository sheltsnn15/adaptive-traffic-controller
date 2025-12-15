// Core/Inc/tlv_parser.hpp
#pragma once
#include "cppcrc.hpp"
#include "traffic_types.hpp"
#include <cstdint>

class TlvParser { // direct byte-to-struct parsing
  public:
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
    const Traffic::LaneCounts &getLaneCounts() const { return lane_counts_; }

    // Reset parser state
    void reset();

    // Statistics
    uint32_t getFramesOk() const { return frames_ok_; }
    uint32_t getCrcFails() const { return crc_fails_; }
    uint32_t getResyncs() const { return resyncs_; }

    // Encoding methods
    static bool encodeLightState(const Traffic::LightStatePayload &cmd,
                                 uint8_t *buffer, size_t &len);
    static bool encodeHeartbeat(const Traffic::HeartBeat &hb, uint8_t *buffer,
                                size_t &len);

  private:
    ParseState state_;
    uint8_t current_msg_type_;
    uint16_t expected_payload_len_;
    uint16_t crc_calculated_;
    uint16_t received_crc_;
    uint8_t payload_bytes_received_;
    uint8_t crc_bytes_received_;

    // Statistics
    uint32_t frames_ok_;
    uint32_t crc_fails_;
    uint32_t resyncs_;

    // Current message being parsed directly into structs
    Traffic::LaneCounts lane_counts_;
    uint32_t timestamp_accumulator_; // For building multi-byte timestamp

    bool validateFrame() const;
    void parseLaneCountsByte(uint8_t byte);
    void resetForResync();
};
