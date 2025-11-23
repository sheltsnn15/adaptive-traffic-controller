// Core/Inc/traffic_controller.hpp
#pragma once
#include "adaptive_state_machine.hpp"
#include "uart_manager.hpp"

class TrafficController {
  private:
    UartManager &uart_;
    AdaptiveStateMachine state_machine_;

    // RTOS task handles
    TaskHandle_t controller_task_ = nullptr;
    TaskHandle_t tx_task_ = nullptr;

    // Timing
    uint32_t last_update_ = 0;
    static constexpr uint32_t CONTROL_PERIOD_MS = 100;

  public:
    TrafficController(UartManager &uart);
    bool start();

  private:
    // RTOS task functions
    static void controllerTask(void *params);
    static void txTask(void *params);

    void runControllerLoop();
    void runTxLoop();
};
