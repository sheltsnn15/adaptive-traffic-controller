// Core/Inc/task_manager.hpp
#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "tlv_parser.hpp"
#include "traffic_state_machine.hpp"
#include "traffic_types.hpp"

class TaskManager {
  public:
    static bool initialize();
    static void startTasks();

    // Queue interfaces
    static bool sendLaneCounts(const LaneCounts &counts);
    static bool sendLightCommand(const LightStatePayload &cmd);

  private:
    static QueueHandle_t lane_counts_queue_;
    static QueueHandle_t light_cmd_queue_;
    static TaskHandle_t uart_rx_task_;
    static TaskHandle_t traffic_ctrl_task_;
    static TaskHandle_t uart_tx_task_;

    // Task functions
    static void uartRxTask(void *params);
    static void trafficCtrlTask(void *params);
    static void uartTxTask(void *params);

    // Task implementations
    static void runUartRx();
    static void runTrafficCtrl();
    static void runUartTx();
};
