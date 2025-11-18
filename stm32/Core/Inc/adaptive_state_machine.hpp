/*
 * adaptive_state_machine.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: shelton
 */

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

    LightState current_state_;
    PhaseTiming timing_;
    LaneCounts last_counts_;

    // Adaptive logic state
    uint8_t extension_count_ = 0;
    bool should_extend_ = false;

  public:
    AdaptiveStateMachine();

    // IStateMachine implementation
    void processSensorData(const LaneCounts &counts) override;
    LightCommand getCurrentCommand() const override;
    void update(uint32_t elapsed_ms) override;

  private:
    void transitionTo(LightState new_state);
    bool checkExtensionCondition(const LaneCounts &counts) const;
    uint32_t getPhaseDuration(LightState state) const;
};
