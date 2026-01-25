// Core/Src/traffic_state_machine.cpp
#include "traffic_state_machine.hpp"
#include <cstdint>

TrafficStateMachine::TrafficStateMachine() = default;

uint32_t
TrafficStateMachine::plannedDuration(TrafficStateMachine::State s) const {
    switch (s) {
    case State::NS_GREEN:
    case State::EW_GREEN:
        return TrafficStateMachine::GREEN_MIN;
    case State::NS_GREEN_EXTENDED:
    case State::EW_GREEN_EXTENDED:
        return TrafficStateMachine::EXTEND_TIME;
    case State::NS_YELLOW:
    case State::EW_YELLOW:
        return TrafficStateMachine::YELLOW_TIME;
    case State::ALL_RED:
        return TrafficStateMachine::ALL_RED_TIME;
    case State::WATCHDOG_TIMEOUT:
    case State::EMERGENCY_RED:
        return 0;
    }
    return 0;
}

void TrafficStateMachine::processLaneCounts(const Traffic::LaneCounts &counts) {
    last_counts_ = counts;
    counts_age_ = 0; // fresh data arrived
    // if we were in watchdog fault, recover automatically
    if (current_state_ == State::WATCHDOG_TIMEOUT) {
        clearFault(); // goes to ALL_RED then resumes NS phase
    }
}

Traffic::LightStatePayload TrafficStateMachine::getCurrentCommand() const {
    Traffic::LightStatePayload cmd{};

    // 1) external lamp state
    Traffic::LightState out = Traffic::LightState::ALL_RED;
    switch (current_state_) {
    case State::NS_GREEN:
    case State::NS_GREEN_EXTENDED:
        out = Traffic::LightState::NS_GREEN;
        break;
    case State::NS_YELLOW:
        out = Traffic::LightState::NS_YELLOW;
        break;
    case State::EW_GREEN:
    case State::EW_GREEN_EXTENDED:
        out = Traffic::LightState::EW_GREEN;
        break;
    case State::EW_YELLOW:
        out = Traffic::LightState::EW_YELLOW;
        break;
    case State::ALL_RED:
    case State::WATCHDOG_TIMEOUT:
    case State::EMERGENCY_RED:
        out = Traffic::LightState::ALL_RED;
        break;
    }

    Traffic::DecisionReason reason = Traffic::DecisionReason::FIXED_TIME;

    if (current_state_ == State::NS_GREEN_EXTENDED ||
        current_state_ == State::EW_GREEN_EXTENDED) {
        reason = Traffic::DecisionReason::ADAPTIVE_EXTENSION;
    } else if (current_state_ == State::WATCHDOG_TIMEOUT ||
               current_state_ == State::EMERGENCY_RED) {
        reason = Traffic::DecisionReason::EMERGENCY; // Fixed: was EMERGENCY_RED
    }

    // 3) phase_duration = remaining time in this sub-state (100ms units)
    uint32_t plan = plannedDuration(current_state_),
             remaining = (plan > state_timer_) ? (plan - state_timer_) : 0;

    cmd.current_state = static_cast<uint8_t>(out);
    cmd.decision_reason = static_cast<uint8_t>(reason);
    cmd.phase_duration = static_cast<uint16_t>(remaining);

    return cmd;
}

void TrafficStateMachine::transitionTo(State new_state) {
    State prev = current_state_;
    current_state_ = new_state;
    state_timer_ = 0;

    switch (new_state) {
    case State::NS_GREEN:
    case State::EW_GREEN:
        // Only reset when starting a NEW NS phase (coming from ALL_RED)
        if (prev == State::ALL_RED) {
            extension_count_ = 0;
            green_total_ = 0; // accumulates across 8s + extensions
        }
        break;
    case State::EW_GREEN_EXTENDED:
    case State::NS_GREEN_EXTENDED:
        // extension_count_ is incremented when we finish the extension chunk
        break;
    default:
        break;
    }
}

void TrafficStateMachine::update(uint32_t elapsed_ms) {
    ms_accum_ += elapsed_ms;
    uint32_t dt = ms_accum_ / TICK_MS;
    ms_accum_ %= TICK_MS;

    while (dt--) {
        state_timer_ += 1;

        // Data freshness watchdog
        if (current_state_ != State::WATCHDOG_TIMEOUT &&
            current_state_ != State::EMERGENCY_RED) {
            counts_age_ += 1;
            if (counts_age_ >= COUNTS_TIMEOUT) {
                triggerWatchdogTimeout();
                return; // stop normal logic this tick
            }
        }

        switch (current_state_) {
        case State::NS_GREEN: {
            green_total_ += 1;
            // Hard max green
            if (green_total_ >= GREEN_MAX) {
                transitionTo(State::NS_YELLOW);
                break;
            }
            // Minimum window finished?
            if (state_timer_ >= GREEN_MIN) {
                should_extend_ = checkExtensionCondition();
                if (should_extend_) {
                    transitionTo(State::NS_GREEN_EXTENDED);
                } else {
                    transitionTo(State::NS_YELLOW);
                }
            }
            break;
        }
        case State::NS_GREEN_EXTENDED: {
            green_total_ += 1;

            if (state_timer_ >= EXTEND_TIME) {
                extension_count_++; // one more extension chunk used
                transitionTo(
                    State::NS_GREEN); // back to base green (re-evaluate later)
            }
            break;
        }
        case State::NS_YELLOW: {
            if (state_timer_ >= YELLOW_TIME) {
                next_is_ns_ = false; // after all-red, go to EW
                transitionTo(State::ALL_RED);
            }
            break;
        }
        case State::EW_GREEN: {
            green_total_ += 1;

            if (green_total_ >= GREEN_MAX) {
                transitionTo(State::EW_YELLOW);
                break;
            }

            if (state_timer_ >= GREEN_MIN) {
                should_extend_ = checkExtensionCondition();
                if (should_extend_) {
                    transitionTo(State::EW_GREEN_EXTENDED);
                } else {
                    transitionTo(State::EW_YELLOW);
                }
            }
            break;
        }
        case State::EW_GREEN_EXTENDED: {
            green_total_ += 1;

            if (state_timer_ >= EXTEND_TIME) {
                extension_count_++;
                transitionTo(State::EW_GREEN);
            }
            break;
        }
        case State::EW_YELLOW: {
            if (state_timer_ >= YELLOW_TIME) {
                next_is_ns_ = true;
                transitionTo(State::ALL_RED);
            }
            break;
        }
        case State::ALL_RED: {
            if (state_timer_ >= ALL_RED_TIME) {
                transitionTo(next_is_ns_ ? State::NS_GREEN : State::EW_GREEN);
            }
            break;
        }
        case State::WATCHDOG_TIMEOUT:
        case State::EMERGENCY_RED:
            // stay until clearFault()
            break;
        }
    }
}

bool TrafficStateMachine::checkExtensionCondition() const {
    const uint32_t ns = static_cast<uint32_t>(last_counts_.n) +
                        static_cast<uint32_t>(last_counts_.s),
                   ew = static_cast<uint32_t>(last_counts_.e) +
                        static_cast<uint32_t>(last_counts_.w);

    const bool in_ns = (current_state_ == State::NS_GREEN),
               in_ew = (current_state_ == State::EW_GREEN);

    if (!(in_ns || in_ew)) {
        return false; // only extend during green
    }

    const uint32_t current = in_ns ? ns : ew, opposite = in_ns ? ew : ns;

    // extend when current direction is clearly busier
    if (current >= (opposite + 3) && extension_count_ < 2) {
        return true;
    }

    return false;
}

void TrafficStateMachine::triggerEmergency() {
    transitionTo(State::EMERGENCY_RED);
}

void TrafficStateMachine::triggerWatchdogTimeout() {
    transitionTo(State::WATCHDOG_TIMEOUT);
}

void TrafficStateMachine::clearFault() {
    // return to a safe known point
    next_is_ns_ = true;
    transitionTo(State::ALL_RED);
}
