// Core/Inc/uart_manager.hpp
#pragma once
#include "FreeRTOS.h"
#include "i_communicator.hpp"
#include "queue.h"
#include "traffic_types.hpp"

class UartManager : public ICommunicator {
  private:
    QueueHandle_t rxQueue_;
    QueueHandle_t txQueue_;
    UART_HandleTypeDef *huart_;
    TlvParser parser_;

  public:
    UartManager(UART_HandleTypeDef *huart);
    ~UartManager();

    bool init();

    // ICommunicator implementation
    bool send(const void *data, size_t len) override;
    bool receive(void *data, size_t len, uint32_t timeout_ms) override;

    // Specific interface for our protocol
    bool receiveLaneCounts(LaneCounts &counts, uint32_t timeout_ms);
    bool sendLightCommand(const LightCommand &cmd);

  private:
    static void uartRxCallback(UART_HandleTypeDef *huart);
    void processRxByte(uint8_t byte);
};
