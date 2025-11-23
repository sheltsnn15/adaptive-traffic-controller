// Core/Src/app_main.cpp
#include "FreeRTOS.h"
#include "main.h"
#include "queue.h"
#include "task.h"
#include "tlv_parser.hpp"
#include "traffic_state_machine.hpp"
#include "traffic_types.hpp"
#include <cstdio>

// Forward declarations to avoid including entire class definitions
extern "C" {
void app_main(void);
void uartRxTask(void *params);
void trafficCtrlTask(void *params);
void uartTxTask(void *params);
}

// Global objects
namespace {
TlvParser tlvParser;
TrafficStateMachine stateMachine;

// FreeRTOS queues
QueueHandle_t lane_counts_queue = nullptr;
QueueHandle_t light_cmd_queue = nullptr;

// Task handles
TaskHandle_t uart_rx_task = nullptr;
TaskHandle_t traffic_ctrl_task = nullptr;
TaskHandle_t uart_tx_task = nullptr;
} // namespace

// UART handle from main.c
extern UART_HandleTypeDef huart2;

// SystemView naming for our tasks
extern "C" void name_traffic_tasks() {
    SEGGER_SYSVIEW_NameResource((U32)uart_rx_task, "UartRx");
    SEGGER_SYSVIEW_NameResource((U32)traffic_ctrl_task, "TrafficCtrl");
    SEGGER_SYSVIEW_NameResource((U32)uart_tx_task, "UartTx");
}

// Initialize FreeRTOS objects
extern "C" bool init_traffic_system() {
    // Create queues
    lane_counts_queue = xQueueCreate(2, sizeof(Traffic::LaneCounts));
    light_cmd_queue = xQueueCreate(1, sizeof(Traffic::LightStatePayload));

    if (!lane_counts_queue || !light_cmd_queue) {
        return false;
    }

    // Create tasks
    BaseType_t result;

    result = xTaskCreate(uartRxTask, "UartRx", 512, nullptr,
                         tskIDLE_PRIORITY + 3, &uart_rx_task);
    if (result != pdPASS)
        return false;

    result = xTaskCreate(trafficCtrlTask, "TrafficCtrl", 512, nullptr,
                         tskIDLE_PRIORITY + 2, &traffic_ctrl_task);
    if (result != pdPASS)
        return false;

    result = xTaskCreate(uartTxTask, "UartTx", 512, nullptr,
                         tskIDLE_PRIORITY + 1, &uart_tx_task);
    if (result != pdPASS)
        return false;

    // Name tasks in SystemView
    name_traffic_tasks();

    return true;
}

// UART Rx Task (High Priority)
extern "C" void uartRxTask(void *params) {
    (void)params;

    uint8_t rx_byte;
    Traffic::LaneCounts counts;

    while (true) {
        // Wait for byte from UART (blocking)
        if (HAL_UART_Receive(&huart2, &rx_byte, 1, portMAX_DELAY) == HAL_OK) {
            // Feed byte to TLV parser
            uint8_t msg_type = tlvParser.parseByte(rx_byte);

            if (msg_type == 0x01) { // LaneCounts message
                counts = tlvParser.getLaneCounts();

                // Send to traffic controller (non-blocking)
                xQueueOverwrite(lane_counts_queue, &counts);
            }
        }
    }
}

// Traffic Controller Task (Medium Priority)
extern "C" void trafficCtrlTask(void *params) {
    (void)params;

    Traffic::LaneCounts counts;
    Traffic::LightStatePayload light_cmd;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t control_period = pdMS_TO_TICKS(100); // 100ms control loop

    while (true) {
        // 1. Check for new lane counts (non-blocking)
        if (xQueueReceive(lane_counts_queue, &counts, 0) == pdTRUE) {
            stateMachine.processLaneCounts(counts);
        }

        // 2. Update state machine
        stateMachine.update(100); // 100ms elapsed

        // 3. Get current light command
        light_cmd = stateMachine.getCurrentCommand();

        // 4. Send to UART Tx task (non-blocking)
        xQueueOverwrite(light_cmd_queue, &light_cmd);

        // 5. Wait for next control period
        vTaskDelayUntil(&last_wake_time, control_period);
    }
}

// UART Tx Task (Low Priority)
extern "C" void uartTxTask(void *params) {
    (void)params;

    Traffic::LightStatePayload light_cmd;
    uint8_t tx_buffer[32];
    size_t tx_len;

    while (true) {
        // Wait for light state to send (blocking)
        if (xQueueReceive(light_cmd_queue, &light_cmd, portMAX_DELAY) ==
            pdTRUE) {
            // Encode as TLV frame
            if (TlvParser::encodeLightState(light_cmd, tx_buffer, tx_len)) {
                // Send via UART
                HAL_UART_Transmit(&huart2, tx_buffer, tx_len, 100);
            }
        }
    }
}

// Heartbeat task (optional)
extern "C" void heartbeatTask(void *params) {
    (void)params;

    Traffic::HeartBeat hb;
    uint8_t tx_buffer[32];
    size_t tx_len;
    uint32_t uptime = 0;

    while (true) {
        hb.uptime_ms = uptime;

        if (TlvParser::encodeHeartbeat(hb, tx_buffer, tx_len)) {
            HAL_UART_Transmit(&huart2, tx_buffer, tx_len, 100);
        }

        uptime += 1000; // 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Main C++ application entry point
extern "C" void app_main(void) {
    printf("Traffic Lab v2 Starting...\r\n");

    if (!init_traffic_system()) {
        printf("ERROR: Failed to initialize traffic system\r\n");
        return;
    }

    printf("Traffic system initialized - tasks running\r\n");

    // Create heartbeat task
    TaskHandle_t heartbeat_task;
    xTaskCreate(heartbeatTask, "Heartbeat", 256, nullptr, tskIDLE_PRIORITY,
                &heartbeat_task);
    SEGGER_SYSVIEW_NameResource((U32)heartbeat_task, "Heartbeat");

    // Let FreeRTOS scheduler take over
    // Tasks will run automatically
}
