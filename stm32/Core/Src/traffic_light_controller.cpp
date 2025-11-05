#include "traffic_light_controller.hpp"
#include <stdint.h>
#include <string.h>

extern "C" {
#include "main.h"
}

namespace TrafficSim {

/**
 * There is a state → currentState.
 * There is a current phase duration → phaseDurationMs.
 * The loop will:
 * 1. Update state machine.
 * 2. Wait for current phase duration.
 * 3. Repeat.
 * Use a task loop
 */

// Constructor
TrafficLightController::TrafficLightController()
    : currentState(LightState::RED), // start in RED
      phaseDurationMs(5000) // initial phase duration (example: 5 seconds)
{
    // You can do additional initialization here if needed
}

// Launch the FreeRTOS task
void TrafficLightController::startTask() {
    xTaskCreate(taskEntry,          // Static wrapper
                "TrafficLightTask", // Task name
                512,                // Stack size (words, not bytes)
                this,               // Parameter → pass 'this' pointer
                1,                  // Priority
                NULL                // Task handle
    );
}

// Static wrapper → needed by FreeRTOS
void TrafficLightController::taskEntry(void *param) {
    TrafficLightController *self = static_cast<TrafficLightController *>(param);
    if (self != nullptr) {
        self->taskLoop();
    }
}

// Main task loop → state machine runs here
void TrafficLightController::taskLoop() {
    for (;;) {
        updateStateMachine();

        // Wait for current phase duration
        vTaskDelay(pdMS_TO_TICKS(phaseDurationMs));
    }
}

// Example updateStateMachine() → simple phase rotation
void TrafficLightController::updateStateMachine() {
    // Simple example: cycle through RED → GREEN → YELLOW → RED ...
    switch (currentState) {
    case LightState::RED:
        currentState = LightState::GREEN;
        phaseDurationMs = 7000; // example: GREEN for 7s
        break;

    case LightState::GREEN:
        currentState = LightState::YELLOW;
        phaseDurationMs = 2000; // YELLOW for 2s
        break;

    case LightState::YELLOW:
        currentState = LightState::RED;
        phaseDurationMs = 5000; // RED for 5s
        break;
    }

    // Send current state to CommHandler
    publishStateMachine();
}

// Placeholder → send currentState over MQTT (to be implemented later)
void TrafficLightController::publishStateMachine() {
    // TODO: call CommHandler to publish light_state message
    // For now, you can print to UART or log for testing
    static const uint8_t redMsg[] = "RED\r\n";
    static const uint8_t greenMsg[] = "GREEN\r\n";
    static const uint8_t yellowMsg[] = "YELLOW\r\n";

    const uint8_t *msg = redMsg;
    uint16_t len = sizeof(msg) - 1; // Reduce string fragmentation

    switch (currentState) {
    case LightState::RED:
        msg = redMsg;
        len = sizeof(redMsg) - 1;
        break;

    case LightState::GREEN:
        msg = greenMsg;
        len = sizeof(greenMsg) - 1;
        break;

    case LightState::YELLOW:
        msg = yellowMsg;
        len = sizeof(yellowMsg) - 1;
        break;
    }
    HAL_UART_Transmit(&huart2, msg, len, HAL_MAX_DELAY);
}

// External setState → can be called when junction_mode changes
void TrafficLightController::setState(LightState newState) {
    currentState = newState;
}

// Getter
LightState TrafficLightController::getCurrentState() const {
    return currentState;
}

void TrafficLightController::setPhaseDuration(uint32_t durationMs) {
    phaseDurationMs = durationMs;
}

} // namespace TrafficSim
