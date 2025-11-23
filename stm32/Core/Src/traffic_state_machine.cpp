// Core/Src/traffic_state_machine.cpp
#include "traffic_state_machine.hpp"

TrafficStateMachine::TrafficStateMachine()
    : current_state_(State::NS_GREEN), state_timer_(0), extension_count_(0),
      should_extend_(false) {}

void TrafficStateMachine::processLaneCounts(const Traffic::LaneCounts &counts) {
}

void TrafficStateMachine::update(uint32_t elapsed_ms) {}

Traffic::LightStatePayload TrafficStateMachine::getCurrentCommand() const {
	Traffic::LightStatePayload cmd{};
	return cmd;
}

bool TrafficStateMachine::checkExtensionCondition() const { return true; }
