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

#include <cstdint>
#include <cstdio>
#include <stddef.h>
#include <stdint.h>

extern "C" void uartRxTask(void *params);
extern "C" void trafficCtrlTask(void *params);
extern "C" void uartTxTask(void *params);
extern "C" void heartbeatTask(void *params);

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
QueueHandle_t tx_frame_queue = nullptr;

constexpr size_t kMaxFrameBytes = 32;
constexpr UBaseType_t kLaneQueueDepth = 1;
constexpr UBaseType_t kRxByteQueueDepth = 128;
constexpr UBaseType_t kTxQueueDepth = 8;

// Time constants
constexpr uint32_t kControlPeriodMs = 100;
constexpr TickType_t kControlPeriodTicks = pdMS_TO_TICKS(kControlPeriodMs);
constexpr TickType_t kHeartbeatPeriodTicks = pdMS_TO_TICKS(2000);
constexpr TickType_t kRxActiveWindowTicks = pdMS_TO_TICKS(1000);

struct TxFrame {
    uint16_t len;
    uint8_t data[kMaxFrameBytes];
};

inline bool enqueue_best_effort(QueueHandle_t q, const TxFrame &frame) {
    if (xQueueSend(q, &frame, 0) == pdTRUE) {
        return true;
    }

    // full: drop one oldest and retry once
    TxFrame drop{};
    (void)xQueueReceive(q, &drop, 0);
    return (xQueueSend(q, &frame, 0) == pdTRUE);
}

inline bool make_light_frame(const Traffic::LightStatePayload &cmd,
                             TxFrame &out) {
    size_t tx_len = sizeof(out.data);
    if (!TlvParser::encodeLightState(cmd, out.data, tx_len))
        return false;
    out.len = static_cast<uint16_t>(tx_len);
    return true;
}

inline bool make_heartbeat_frame(const Traffic::HeartBeat &hb, TxFrame &out) {
    size_t tx_len = sizeof(out.data);
    if (!TlvParser::encodeHeartbeat(hb, out.data, tx_len))
        return false;
    out.len = static_cast<uint16_t>(tx_len);
    return true;
}

inline bool create_task(TaskFunction_t fn, const char *name,
                        uint16_t stackWords, UBaseType_t prio,
                        TaskHandle_t *outHandle) {
    BaseType_t ok = xTaskCreate(fn, name, stackWords, nullptr, prio, outHandle);
    if (ok != pdPASS) {
        printf("ERROR: %s task creation failed\r\n", name);
        return false;
    }
    return true;
}

// heartbeat link-ok detection
static volatile TickType_t last_lane_rx_tick = 0;

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
    if (traffic_ctrl_task)
        SEGGER_SYSVIEW_NameResource((U32)traffic_ctrl_task, "TrafficCtrl");
    if (uart_tx_task)
        SEGGER_SYSVIEW_NameResource((U32)uart_tx_task, "UartTx");
}

static void SysViewControlTask(void *arg) {
    (void)arg;

    SEGGER_SYSVIEW_Start();         // start recording from task context
    vTaskDelay(pdMS_TO_TICKS(1));   // 1 tick is enough
    name_traffic_tasks();           // emits NameResource events (now recorded)
    SEGGER_SYSVIEW_SendTaskList();  // makes Context list resolve nicely
    vTaskDelay(pdMS_TO_TICKS(300)); // capture window (tune this)
    SEGGER_SYSVIEW_Stop();

    // __asm volatile("bkpt #0");      // Don't halt the MCU
    vTaskDelete(NULL);
}

// Initialize FreeRTOS objects
extern "C" bool init_traffic_system() {
    // Create queues (small sizes - we only need latest data)
    lane_counts_queue =
        xQueueCreate(kLaneQueueDepth, sizeof(Traffic::LaneCounts));
    uart_rx_byte_q = xQueueCreate(kRxByteQueueDepth, sizeof(uint8_t));
    tx_frame_queue = xQueueCreate(kTxQueueDepth, sizeof(TxFrame));

    if (!lane_counts_queue || !uart_rx_byte_q || !tx_frame_queue) {
        printf("ERROR: Queue creation failed\r\n");
        return false;
    }

    // Create tasks
    if (!create_task(uartRxTask, "UartRx", 1024, tskIDLE_PRIORITY + 3,
                     &uart_rx_task))
        return false;
    if (!create_task(trafficCtrlTask, "TrafficCtrl", 1024, tskIDLE_PRIORITY + 2,
                     &traffic_ctrl_task))
        return false;
    if (!create_task(uartTxTask, "UartTx", 1024, tskIDLE_PRIORITY + 1,
                     &uart_tx_task))
        return false;
    if (!create_task(heartbeatTask, "Heartbeat", 512, tskIDLE_PRIORITY,
                     &heartbeat_task))
        return false;

    // Optional: SysView control task can ignore handle
    (void)xTaskCreate(SysViewControlTask, "SysViewCtl", 256, nullptr,
                      tskIDLE_PRIORITY + 4, nullptr);

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
                last_lane_rx_tick = xTaskGetTickCount();

                if (stateMachine.isWatchdogTimeout()) {
                    stateMachine.clearFault();
                }

                // printf("received a lane count\r\n");
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

    while (true) {
        // 1. Check for new lane counts (non-blocking)
        if (xQueueReceive(lane_counts_queue, &counts, 0) == pdTRUE) {
            stateMachine.processLaneCounts(counts);
        }

        // 2. Update state machine
        stateMachine.update(kControlPeriodMs);

        // 3. Get current light command
        light_cmd = stateMachine.getCurrentCommand();

        // 4. Encode and send as TxFrame (non-blocking, best-effort)
        TxFrame frame{};
        if (make_light_frame(light_cmd, frame)) {
            (void)enqueue_best_effort(tx_frame_queue, frame);
        }

        // 5. Wait for next control period (100ms)
        vTaskDelayUntil(&last_wake_time, kControlPeriodTicks);
    }
}

// UART Tx Task (Low Priority)
extern "C" void uartTxTask(void *params) {
    (void)params;
    TxFrame frame{};

    while (true) {
        if (xQueueReceive(tx_frame_queue, &frame, portMAX_DELAY) == pdTRUE) {
            HAL_UART_Transmit(&huart2, frame.data, frame.len, 1000);
        }
    }
}

// Heartbeat task - sends every 2 seconds
extern "C" void heartbeatTask(void *params) {
    (void)params;
    Traffic::HeartBeat hb{};
    TickType_t last_wake_time = xTaskGetTickCount();
    static uint16_t hb_seq = 0;

    while (true) {
        hb.uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        hb.seq = hb_seq++;

        uint8_t st = Traffic::HB_OK;

        // link-ok / RX active if we saw LaneCounts recently
        TickType_t now = xTaskGetTickCount();
        if ((now - last_lane_rx_tick) < kRxActiveWindowTicks) {
            st |= Traffic::HB_UART_RX_ACTIVE;
        }

        if (tlvParser.getCrcFails() > 0) {
            st |= Traffic::HB_CRC_FAIL_SEEN;
        }

        if (tlvParser.getResyncs() > 0) {
            st |= Traffic::HB_RESYNC_SEEN;
        }

        hb.status = st;

        TxFrame frame{};
        if (make_heartbeat_frame(hb, frame)) {
            (void)enqueue_best_effort(tx_frame_queue, frame);
            printf("HB %lu seq=%u st=0x%02X\r\n", (unsigned long)hb.uptime_ms,
                   (unsigned)hb.seq, (unsigned)hb.status);
        }

        vTaskDelayUntil(&last_wake_time, kHeartbeatPeriodTicks);
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
