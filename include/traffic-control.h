#ifndef TRAFFIC_CONTROL_H_ /* Include guard */
#define TRAFFIC_CONTROL_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Traffic Light Output Constants
#define GREEN_NS_RED_EW 0x21     // North-South Green, East-West Red
#define YELLOW_NS_RED_EW 0x22    // North-South Yellow, East-West Red
#define RED_NS_GREEN_EW 0x0C     // North-South Red, East-West Green
#define RED_NS_YELLOW_EW 0x14    // North-South Red, East-West Yellow
#define GREEN_NS_TURN_ARROW 0x29 // North-South Green with Turn Arrow

// UART Configuration for ESP32
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define TXD_PIN (GPIO_NUM_17) // UART Transmit pin
#define RXD_PIN (GPIO_NUM_16) // UART Receive pin

// Enum for Traffic Light States
typedef enum {
  GO_N,   // North Green
  WAIT_N, // North Yellow
  GO_E,   // East Green
  WAIT_E, // East Yellow
  GO_S,   // South Green
  WAIT_S, // South Yellow
  GO_W,   // West Green
  WAIT_W  // West Yellow
} TrafficState;

// Enum for Junction Types
typedef enum {
  JUNCTION_Y, // Y-Junction
  JUNCTION_X, // X-Junction
  JUNCTION_H  // H-Junction
} JunctionType;

// State Structure for Finite State Machine
typedef const struct {
  unsigned long Out;    // Output signal for the current state
  unsigned long Time;   // Time delay for this state in milliseconds
  TrafficState Next[4]; // Next states based on input
} STyp;

// External declarations of State Machines for each Junction
extern STyp FSM_Y[8]; // State Machine array for Y-Junction
extern STyp FSM_X[4]; // State Machine array for X-Junction
extern STyp FSM_H[5]; // State Machine array for H-Junction

// Variables to keep track of current state for each junction
extern unsigned long Y_JunctionState;
extern unsigned long X_JunctionState;
extern unsigned long H_JunctionState;

// Example input variables to simulate traffic or sensor data
extern unsigned long input_Y; // Input for Y-Junction
extern unsigned long input_X; // Input for X-Junction
extern unsigned long input_H; // Input for H-Junction

// Queue handle for traffic state data
extern QueueHandle_t trafficStateQueue;

// Function Prototypes
/**
 * @brief Generates random traffic input for simulation purposes.
 */
void generateRandomTraffic(void);

/**
 * @brief Prints the current state of the traffic light for a specific junction.
 *
 * @param junctionType Type of the junction (Y, X, or H).
 * @param lightState Output state of the traffic light.
 */
void printTrafficLightState(JunctionType junctionType,
                            unsigned long lightState);

/**
 * @brief Manages the state transitions for a specific traffic light junction.
 *
 * @param junctionType Type of the junction (JUNCTION_Y, JUNCTION_X,
 * JUNCTION_H).
 * @param currentState Pointer to the current state of the junction.
 * @param fsm Pointer to the finite state machine array for the junction.
 * @param oldOutput Pointer to the previous output state.
 * @param input Sensor input or traffic condition for decision making.
 */
void handleJunctionState(JunctionType junctionType, unsigned long *currentState,
                         STyp *fsm, unsigned long *oldOutput,
                         unsigned long input);

/**
 * @brief Main traffic light control task for managing multiple junctions.
 *
 * @param pvParameter FreeRTOS task parameter (unused).
 */
void trafficLightTask(void *pvParameter);

/**
 * @brief Initializes the UART for communication.
 */
void uart_init(void);

/**
 * @brief Sends data over UART.
 *
 * @param data Pointer to the string data to be sent.
 */
void uart_send_data(const char *data);

/**
 * @brief Main application entry point.
 */
void app_main(void);

#endif // TRAFFIC_CONTROL_H_
