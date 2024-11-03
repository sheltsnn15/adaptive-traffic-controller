#include "traffic-control.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

// Logger tag
static const char *TAG = "traffic_light";

// Initialize state machines for each junction
STyp FSM_Y[8] = {
    {GREEN_NS_RED_EW,
     3000,
     {WAIT_N, WAIT_N, WAIT_N,
      WAIT_N}}, // Y Junction, North-South Green, East Red
    {YELLOW_NS_RED_EW,
     500,
     {GO_E, GO_E, GO_E, GO_E}}, // Y Junction, North-South Yellow, East Red
    {RED_NS_GREEN_EW,
     3000,
     {WAIT_E, WAIT_E, WAIT_E,
      WAIT_E}}, // Y Junction, North-South Red, East Green
    {RED_NS_YELLOW_EW,
     500,
     {GO_N, GO_N, GO_N, GO_N}}, // Y Junction, North-South Red, East Yellow
    {GREEN_NS_RED_EW,
     3000,
     {WAIT_N, WAIT_N, WAIT_N,
      WAIT_N}}, // Y Junction, North Green, South-East Red
    {YELLOW_NS_RED_EW,
     500,
     {GO_S, GO_S, GO_S, GO_S}}, // Y Junction, North Yellow, South-East Red
    {RED_NS_GREEN_EW,
     3000,
     {WAIT_S, WAIT_S, WAIT_S,
      WAIT_S}}, // Y Junction, North Red, South-East Green
    {RED_NS_YELLOW_EW,
     500,
     {GO_N, GO_N, GO_N, GO_N}} // Y Junction, North Red, South-East Yellow
};

STyp FSM_X[4] = {
    {GREEN_NS_RED_EW,
     3000,
     {WAIT_N, WAIT_N, WAIT_N,
      WAIT_N}}, // X Junction, North-South Green, East-West Red
    {YELLOW_NS_RED_EW,
     500,
     {GO_E, GO_E, GO_E, GO_E}}, // X Junction, North-South Yellow, East-West Red
    {RED_NS_GREEN_EW,
     3000,
     {WAIT_E, WAIT_E, WAIT_E,
      WAIT_E}}, // X Junction, North-South Red, East-West Green
    {RED_NS_YELLOW_EW,
     500,
     {GO_N, GO_N, GO_N, GO_N}} // X Junction, North-South Red, East-West Yellow
};

STyp FSM_H[5] = {
    {GREEN_NS_RED_EW,
     4000,
     {WAIT_N, WAIT_N, WAIT_N,
      WAIT_N}}, // H Junction, North-South Green, East-West Red
    {YELLOW_NS_RED_EW,
     1000,
     {GO_E, GO_E, GO_E, GO_E}}, // H Junction, North-South Yellow, East-West Red
    {RED_NS_GREEN_EW,
     3000,
     {WAIT_E, WAIT_E, WAIT_E,
      WAIT_E}}, // H Junction, North-South Red, East-West Green
    {RED_NS_YELLOW_EW,
     1000,
     {GO_N, GO_N, GO_N, GO_N}}, // H Junction, North-South Red, East-West Yellow
    {GREEN_NS_TURN_ARROW,
     2000,
     {WAIT_N, WAIT_N, WAIT_N,
      WAIT_N}} // H Junction, North-South Green with Turn Arrow
};

// Current state variables for each junction
unsigned long Y_JunctionState = 0;
unsigned long X_JunctionState = 0;
unsigned long H_JunctionState = 0;

// Input variables for each junction (for simulation)
unsigned long input_Y = 0;
unsigned long input_X = 0;
unsigned long input_H = 0;

void generateRandomTraffic(void) {
  input_Y = esp_random() % 4; // Simulate random traffic input for Y-Junction
  input_X = esp_random() % 4; // Simulate random traffic input for X-Junction
  input_H = esp_random() % 4; // Simulate random traffic input for H-Junction
}

void printTrafficLightState(JunctionType junctionType,
                            unsigned long lightState) {
  ESP_LOGI(TAG, "-------------------------------");
  switch (junctionType) {
  case JUNCTION_X:
    ESP_LOGI(TAG, "Junction Type: X-Junction");
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");
    break;
  case JUNCTION_Y:
    ESP_LOGI(TAG, "Junction Type: Y-Junction");
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");

    ESP_LOGI(TAG, "South-East Green Light: %s",
             (lightState & 0x03) ? "On" : "Off");
    ESP_LOGI(TAG, "South-East Yellow Light: %s",
             (lightState & 0x04) ? "On" : "Off");
    ESP_LOGI(TAG, "South-East Red Light: %s",
             (lightState & 0x01) ? "On" : "Off");
    break;
  case JUNCTION_H:
    ESP_LOGI(TAG, "Junction Type: H-Junction");
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");
    ESP_LOGI(TAG, "North Turn Arrow: %s", (lightState & 0x01) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");
    break;
  default:
    ESP_LOGW(TAG, "Unknown junction type");
  }
}

void handleJunctionState(JunctionType junctionType, unsigned long *currentState,
                         STyp *fsm, unsigned long *oldOutput,
                         unsigned long input) {
  *oldOutput = fsm[*currentState].Out;
  printTrafficLightState(junctionType, *oldOutput);
  vTaskDelay(pdMS_TO_TICKS(fsm[*currentState].Time));
  *currentState = fsm[*currentState].Next[input];
}

void trafficLightTask(void *pvParameter) {
  unsigned long oldOutput_Y = 0, oldOutput_X = 0, oldOutput_H = 0;

  while (1) {
    generateRandomTraffic();

    // Handle each junction state based on current inputs
    handleJunctionState(JUNCTION_Y, &Y_JunctionState, FSM_Y, &oldOutput_Y,
                        input_Y);
    handleJunctionState(JUNCTION_X, &X_JunctionState, FSM_X, &oldOutput_X,
                        input_X);
    handleJunctionState(JUNCTION_H, &H_JunctionState, FSM_H, &oldOutput_H,
                        input_H);
  }
}

void uart_init(void) {
  const uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);
  uart_param_config(UART_PORT_NUM, &uart_config);
  uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
}

void uart_send_data(const char *data) {
  uart_write_bytes(UART_PORT_NUM, data, strlen(data));
}

void app_main(void) {
  // Initialize UART
  uart_init();

  // Create the traffic light task
  xTaskCreate(trafficLightTask, "trafficLightTask", 2048, NULL, 5, NULL);
}
