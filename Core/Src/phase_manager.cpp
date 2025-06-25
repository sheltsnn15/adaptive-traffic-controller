/*
 * phase_manager.cpp
 *
 *  Created on: Jun 22, 2025
 *      Author: shelton
 */

#include "phase_manager.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "traffic_data_handler.hpp"
#include "traffic_light_controller.hpp"

namespace TrafficSim {

static PhaseState currentPhase;
static JunctionConfig junctionConfig; // Populated at init

void PhaseManager::init(const JunctionConfig &config) {
    junctionConfig = config;
    // Set default phase (e.g., N+S)
    currentPhase.activeLanes[0] = LaneID::NORTHBOUND;
    currentPhase.activeLanes[1] = LaneID::SOUTHBOUND;
    currentPhase.greenStartTick = xTaskGetTickCount();
    currentPhase.lastVehicleSum = 0;
}

void PhaseManager::runLoop() {
    for (;;) {
        TrafficData data;
        if (TrafficDataHandler::receiveTrafficData(data, pdMS_TO_TICKS(100))) {
            // Update internal model logic
        }

        // Evaluate whether we should switch phases
        // If yes: notify traffic_light_controller
    }
}

bool PhaseManager::shouldSwitchPhase() {
    // Return true if conditions met
}

} /* namespace TrafficSim */
