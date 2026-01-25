// Core/Inc/traffic_state_machine.hpp
#pragma once
#include "traffic_types.hpp"

class TrafficStateMachine {
  public:
    enum class State {
        NS_GREEN,
        NS_GREEN_EXTENDED,
        NS_YELLOW,
        ALL_RED,
        EW_GREEN,
        EW_GREEN_EXTENDED,
        EW_YELLOW,
        WATCHDOG_TIMEOUT,
        EMERGENCY_RED
    };

    TrafficStateMachine();

    // Process new traffic data
    void processLaneCounts(const Traffic::LaneCounts &counts);

    // Update state machine (call every 100ms)
    void update(uint32_t elapsed_ms);

    // Get current command
    Traffic::LightStatePayload getCurrentCommand() const;

    // Emergency and watchdog controls
    void triggerEmergency();
    void triggerWatchdogTimeout();
    void clearFault();
    bool isWatchdogTimeout() const noexcept {
        return current_state_ == State::WATCHDOG_TIMEOUT;
    }

  private:
    State current_state_{State::NS_GREEN};
    uint32_t state_timer_{0}, // time spent in current state
        extension_count_{0},  // number of 4s extensions applied in this green
        green_total_{0}, // total green time for current direction (100ms units)
        counts_age_{0}, ms_accum_{0};
    Traffic::LaneCounts last_counts_{};
    bool should_extend_{false},
        next_is_ns_{true}; // which phase to run after ALL_RED

    // Timing constants (in 100ms units)
    static constexpr uint32_t GREEN_MIN = 80, // 8s
        GREEN_MAX = 250,                      // 25s
        YELLOW_TIME = 30,                     // 3s
        ALL_RED_TIME = 10,                    // 1s
        EXTEND_TIME = 40,                     // 4s
        COUNTS_TIMEOUT = 30,                  // 3s
        TICK_MS = 100;                        // Timebase

    void transitionTo(State new_state);
    bool checkExtensionCondition() const;
    uint32_t plannedDuration(State s) const;
};
