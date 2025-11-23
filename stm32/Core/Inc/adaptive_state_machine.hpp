// Core/Inc/adaptive_state_machine.hpp
#pragma once
#include "i_state_machine.hpp"
#include "traffic_types.hpp"

class AdaptiveStateMachine : public IStateMachine {
  private:
    struct PhaseTiming {
        uint32_t min_duration;
        uint32_t max_duration;
        uint32_t elapsed;
    };

    Traffic::LightState current_state_;
    PhaseTiming timing_;
    Traffic::LaneCounts last_counts_;

    // Adaptive logic state
    uint8_t extension_count_ = 0;
    bool should_extend_ = false;

  public:
    AdaptiveStateMachine();

    // IStateMachine implementation
    void processSensorData(const Traffic::LaneCounts &counts) override;
    Traffic::LightStatePayload getCurrentCommand() const override;
    void update(uint32_t elapsed_ms) override;

  private:
    void transitionTo(Traffic::LightState new_state);
    bool checkExtensionCondition(const Traffic::LaneCounts &counts) const;
    uint32_t getPhaseDuration(Traffic::LightState state) const;
};
