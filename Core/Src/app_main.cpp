#include "FreeRTOS.h"
#include "task.h"
#include "traffic_light_controller.hpp"

extern "C" void app_main() {
    static TrafficSim::TrafficLightController controller;
    controller.startTask();
}
