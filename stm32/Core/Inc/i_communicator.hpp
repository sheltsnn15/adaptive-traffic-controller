// Core/Inc/i_communicator.hpp
#pragma once
#include "traffic_types.hpp"

class ICommunicator {
  public:
    virtual ~ICommunicator() = default;
    virtual bool send(const void *data, size_t len) = 0;
    virtual bool receive(void *data, size_t len, uint32_t timeout_ms) = 0;
};
