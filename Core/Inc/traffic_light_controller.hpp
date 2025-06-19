#pragma once

#include "FreeRTOS.h"
#include "timers.h"
#include <cstdint>

namespace TrafficSim {
enum class LightState { RED, YELLOW, GREEN };

class TrafficLightController {
  public:
    void startTask(); // Launch the FreeRTOS task
    TrafficLightController();
    void setState(LightState newState);
    LightState getCurrentState() const;
    void setPhaseDuration(uint32_t durationMs);

  private:
    static void taskEntry(void *param); // static wrapper for FreeRTOS task
    void taskLoop();                    // state machine task loop
    void updateStateMachine();          // Update state based on current logic
    void publishStateMachine();         // Send current state to CommHandler
    LightState currentState;
    uint32_t phaseDurationMs; // Current phase duration in ms
};
} // namespace TrafficSim
