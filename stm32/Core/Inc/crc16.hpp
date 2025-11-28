#pragma once
#include "cppcrc.hpp"

class Crc16 {
  public:
    // Initialize CRC calculation
    static constexpr uint16_t calculate(const uint8_t *data, size_t length) {
        return CRC16::CCITT_FALSE::calc(data, length);
    }

    // Continue CRC calculation (for incremental parsing)
    static constexpr uint16_t
    continue_calc(uint16_t current_crc, const uint8_t *data, size_t length) {
        return CRC16::CCITT_FALSE::calc(data, length, current_crc);
    }

    // Calculate CRC for a single byte
    static constexpr uint16_t calculate_byte(uint8_t byte,
                                             uint16_t current_crc = 0xFFFF) {
        return CRC16::CCITT_FALSE::calc(&byte, 1, current_crc);
    }
};
