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

  private:
    State current_state_;
    uint32_t state_timer_;
    uint32_t extension_count_;
    Traffic::LaneCounts last_counts_;
    bool should_extend_;

    // Timing constants (in 100ms units)
    static constexpr uint32_t NS_GREEN_MIN = 80;  // 8s
    static constexpr uint32_t NS_GREEN_MAX = 250; // 25s
    static constexpr uint32_t YELLOW_TIME = 30;   // 3s
    static constexpr uint32_t ALL_RED_TIME = 10;  // 1s
    static constexpr uint32_t EXTEND_TIME = 40;   // 4s

    void transitionTo(State new_state);
    bool checkExtensionCondition() const;
    uint32_t getCurrentMaxTime() const;
};
