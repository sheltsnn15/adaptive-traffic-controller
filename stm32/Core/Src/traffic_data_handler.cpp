/*
 * traffic_data_handler.cpp
 *
 *  Created on: Jun 22, 2025
 *      Author: shelton
 */

#include "traffic_data_handler.hpp"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>

namespace TrafficSim {

static QueueHandle_t trafficDataQueue = nullptr;

void TrafficDataHandler::initQueue() {
    trafficDataQueue = xQueueCreate(20, sizeof(TrafficData));
}

bool TrafficDataHandler::sendTrafficData(const TrafficData &data) {
    if (trafficDataQueue == nullptr)
        return false;
    return xQueueSend(trafficDataQueue, &data, 0) == pdPASS;
}

bool TrafficDataHandler::receiveTrafficData(TrafficData &outData,
                                            TickType_t timeout) {
    if (trafficDataQueue == nullptr)
        return false;
    return xQueueReceive(trafficDataQueue, &outData, timeout) == pdPASS;
}

} /* namespace TrafficSim */
