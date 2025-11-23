// Core/Inc/traffic_types.hpp
#pragma once
#include <cstdint>

namespace Traffic {
enum class LightState : uint8_t {
    NS_GREEN = 0,
    NS_YELLOW = 1,
    EW_GREEN = 2,
    EW_YELLOW = 3,
    ALL_RED = 4
};

enum class DecisonReason : uint8_t {
    FIXED_TIME = 0,
    ADAPTIVE_EXTENSION = 1,
    EMERGENCY = 2
};

#pragma pack(push, 1) // Saves the current packing alignment and sets it to
                      // 1-byte alignment (no padding)
struct LaneCounts {
    uint32_t ts_sec;       // UNIX seconds
    uint8_t junction_type; // 0=X, 1=Y
    uint8_t n, s, e, w;    // Vehicle counts
};

struct LightStatePayload {
    uint8_t current_state;   // LightState enum
    uint8_t decision_reason; // DecisionReason enum
    uint16_t phase_duration; // in 100ms units
};

struct HeartBeat {
    uint32_t uptime_ms; // Sys uptime
};

#pragma pack(pop) // Restores the previous packing alignment

} // namespace Traffic
