#include "web-server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *TAG = "WebServer";

/**
 * @brief Initializes and starts the web server task.
 */
void start_webserver(uint16_t port) {}

/**
 * @brief Processes a single client request.
 */
void handle_client(int client_socket) {
  char request[WEBSERVER_BUFFER_SIZE];
  char response[WEBSERVER_BUFFER_SIZE];
  int request_len;

  // Read the HTTP request
  request_len = recv(client_socket, request, WEBSERVER_BUFFER_SIZE - 1, 0);
  if (request_len <= 0) {
    ESP_LOGW(TAG, "Failed to read client's request");
    close(client_socket);
    return;
  }

  request[request_len] = '\0'; // Null-terminate the request
  ESP_LOGI(TAG, "Client's request:", request);
  // Check if the request is /traffic-state
  if (strstr(request, "GET /traffic-state") != NULL) {
    int json_len = get_traffic_state_json(response, WEBSERVER_BUFFER_SIZE);
    // Send the reponse with JSON format
    snprintf(response, WEBSERVER_BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s",
             response);
  }
}

/**
 * @brief Fetches the latest traffic state data and formats it as JSON.
 */
int get_traffic_state_json(char *buffer, size_t buffer_size) {
  unsigned long trafficState;
  // Try to recieve data from the queue (non-blocking)
  if (xQueueReceive(trafficStateQueue, &trafficState, 0) == pdTRUE) {
    // Format the traffic state into JSON
    return sprintf(
        buffer, buffer_size,
        "{\"Y_Junction\":%lu,\"H_Junction\":%lu,\"H_Junction\":%lu}");
  } else {
    return sprintf(buffer, buffer_size, "{\"error\":\"No data available\"}");
  }
}
