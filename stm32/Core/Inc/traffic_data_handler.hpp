/*
 * traffic_data_handler.h
 *
 *  Created on: Jun 22, 2025
 *      Author: shelton
 */

#ifndef TRAFFIC_DATA_HANDLER_H_
#define TRAFFIC_DATA_HANDLER_H_

namespace TrafficSim {

enum class LaneID : uint8_t { NORTHBOUND, SOUTHBOUND, EASTBOUND, WESTBOUND };

typedef struct {
    TickType_t lastUpdatedTick;
    uint8_t vehicleCount;
    LaneID laneID;
} TrafficData;

class TrafficDataHandler {
  public:
    void initQueue();
    bool sendTrafficData(const TrafficData &data);
    bool receiveTrafficData(TrafficData &outData, TickType_t timeout);
};

} /* namespace TrafficSim */

#endif /* TRAFFIC_DATA_HANDLER_H_ */
