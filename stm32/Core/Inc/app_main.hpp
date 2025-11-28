// Core/Inc/app_main.hpp
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public function declarations
void app_main(void);
bool init_traffic_system(void);
void name_traffic_tasks(void);

// Task function declarations
void uartRxTask(void *params);
void trafficCtrlTask(void *params);
void uartTxTask(void *params);
void heartbeatTask(void *params);

#ifdef __cplusplus
}
#endif
