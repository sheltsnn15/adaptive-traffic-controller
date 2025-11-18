// Core/Inc/i_state_machine.hpp
#pragma once
#include "traffic_types.hpp"

class IStateMachine {
  public:
    virtual ~IStateMachine() = default;
    virtual void processSensorData(const LaneCounts &counts) = 0;
    virtual LightCommand getCurrentCommand() const = 0;
    virtual void update(uint32_t elapsed_ms) = 0;
};
