/*
 * phase_manager.hpp
 *
 *  Created on: Jun 22, 2025
 *      Author: shelton
 */

#ifndef PHASE_MANAGER_HPP_
#define PHASE_MANAGER_HPP_

#include "traffic_data_handler.hpp"
#include "traffic_light_controller.hpp"
namespace TrafficSim {

enum class JunctionType : uint8_t { X_JUNCTION, Y_JUNCTION };

typedef struct {
    JunctionType type;
    LaneID activeJunctions[2];
    TickType_t minGreenTime;
    TickType_t maxGreenTime;
} JunctionConfig;

typedef struct {
    LaneID activeLanes[2]; // e.g., N+S
    TickType_t greenStartTick;
    uint32_t lastVehicleSum;
} PhaseState;

class PhaseManager {
  public:
    PhaseManager();
    virtual ~PhaseManager();
    void init(const JunctionConfig &config);
    void runLoop();
    bool shouldSwitchPhase();
};

} /* namespace TrafficSim */

#endif /* PHASE_MANAGER_HPP_ */
