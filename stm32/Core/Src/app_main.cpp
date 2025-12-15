// Core/Src/app_main.cpp
#include "app_main.hpp"
#include "FreeRTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "main.h"
#include "queue.h"
#include "task.h"
#include "tlv_parser.hpp"
#include "traffic_state_machine.hpp"
#include "traffic_types.hpp"
#include <cstdio>

extern "C" {
QueueHandle_t uart_rx_byte_q = nullptr;
uint8_t rx_it_byte = 0;
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
TaskHandle_t heartbeat_task = nullptr;
} // namespace

// UART handle from main.c
extern UART_HandleTypeDef huart2;

// SystemView naming for our tasks
extern "C" void name_traffic_tasks() {
    if (uart_rx_task)
        SEGGER_SYSVIEW_NameResource((U32)uart_rx_task, "UartRx");
    if (heartbeat_task)
        SEGGER_SYSVIEW_NameResource((U32)heartbeat_task, "Heartbeat");
    // if (traffic_ctrl_task)
    //     SEGGER_SYSVIEW_NameResource((U32)traffic_ctrl_task, "TrafficCtrl");
    // if (uart_tx_task)
    //     SEGGER_SYSVIEW_NameResource((U32)uart_tx_task, "UartTx");
}

static void SysViewControlTask(void *arg) {
    (void)arg;

    SEGGER_SYSVIEW_Start(); // start recording from task context

    vTaskDelay(pdMS_TO_TICKS(1));  // 1 tick is enough
    name_traffic_tasks();          // emits NameResource events (now recorded)
    SEGGER_SYSVIEW_SendTaskList(); // makes Context list resolve nicely

    vTaskDelay(pdMS_TO_TICKS(300)); // capture window (tune this)
    SEGGER_SYSVIEW_Stop();

    __asm volatile("bkpt #0");
    for (;;) {
    }
}

// Initialize FreeRTOS objects
extern "C" bool init_traffic_system() {
    // Create queues (small sizes - we only need latest data)
    lane_counts_queue = xQueueCreate(1, sizeof(Traffic::LaneCounts));
    light_cmd_queue = xQueueCreate(1, sizeof(Traffic::LightStatePayload));
    uart_rx_byte_q = xQueueCreate(128, sizeof(uint8_t));

    if (!lane_counts_queue || !light_cmd_queue || !uart_rx_byte_q) {
        printf("ERROR: Queue creation failed\r\n");
        return false;
    }

    // Create tasks
    BaseType_t result;

    result = xTaskCreate(uartRxTask, "UartRx", 1024, nullptr,
                         tskIDLE_PRIORITY + 3, &uart_rx_task);
    if (result != pdPASS) {
        printf("ERROR: UartRx task creation failed\r\n");
        return false;
    }

    // result = xTaskCreate(trafficCtrlTask, "TrafficCtrl", 1024, nullptr,
    //                      tskIDLE_PRIORITY + 2, &traffic_ctrl_task);
    // if (result != pdPASS) {
    //     printf("ERROR: TrafficCtrl task creation failed\r\n");
    //     return false;
    // }
    //
    // result = xTaskCreate(uartTxTask, "UartTx", 1024, nullptr,
    //                      tskIDLE_PRIORITY + 1, &uart_tx_task);
    // if (result != pdPASS) {
    //     printf("ERROR: UartTx task creation failed\r\n");
    //     return false;
    // }

    result = xTaskCreate(heartbeatTask, "Heartbeat", 512, nullptr,
                         tskIDLE_PRIORITY, &heartbeat_task);
    if (result != pdPASS) {
        printf("ERROR: Heartbeat task creation failed\r\n");
        return false;
    }

    xTaskCreate(SysViewControlTask, "SysViewCtl", 256, nullptr,
                tskIDLE_PRIORITY + 4, nullptr);

    // Name tasks in SystemView
    name_traffic_tasks();
    printf("All tasks created successfully\r\n");
    return true;
}

// UART Rx Task (High Priority)
extern "C" void uartRxTask(void *params) {
    (void)params;

    uint8_t rx_byte{};
    Traffic::LaneCounts counts{};

    // Start 1-byte interrupt reception
    HAL_UART_Receive_IT(&huart2, &rx_it_byte, 1);

    while (true) {
        // Block until a byte arrives from ISR
        if (xQueueReceive(uart_rx_byte_q, &rx_byte, portMAX_DELAY) == pdTRUE) {
            uint8_t msg_type = tlvParser.parseByte(rx_byte);

            if (msg_type == 0x01) {
                counts = tlvParser.getLaneCounts();
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
    const TickType_t control_period = pdMS_TO_TICKS(100);

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

        // 5. Wait for next control period (100ms)
        vTaskDelayUntil(&last_wake_time, control_period);
    }
}

// UART Tx Task (Low Priority)
extern "C" void uartTxTask(void *params) {
    (void)params;

    Traffic::LightStatePayload light_cmd;
    uint8_t tx_buffer[32];
    size_t tx_len = {};

    while (true) {
        // Wait for light state to send (blocking)
        if (xQueueReceive(light_cmd_queue, &light_cmd, portMAX_DELAY) ==
            pdTRUE) {
            tx_len = sizeof(tx_buffer);
            // Encode as TLV frame
            if (TlvParser::encodeLightState(light_cmd, tx_buffer, tx_len)) {
                // Send via UART
                HAL_UART_Transmit(&huart2, tx_buffer, tx_len, 1000);
            }
        }
    }
}

// Heartbeat task - sends every 2 seconds
extern "C" void heartbeatTask(void *params) {
    (void)params;

    Traffic::HeartBeat hb{};
    uint8_t tx_buffer[32];
    size_t tx_len{};
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t heartbeat_period = pdMS_TO_TICKS(200);

    while (true) {
        printf("HB %lu\r\n", (unsigned long)hb.uptime_ms);

        hb.uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        tx_len = sizeof(tx_buffer);

        if (TlvParser::encodeHeartbeat(hb, tx_buffer, tx_len)) {
            HAL_UART_Transmit(&huart2, tx_buffer, tx_len, 100);
        }

        vTaskDelayUntil(&last_wake_time, heartbeat_period);
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
    // Let FreeRTOS scheduler take over
    // Tasks will run automatically
}
