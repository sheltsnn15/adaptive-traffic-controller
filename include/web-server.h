#ifndef WEBSERVER_H_ /* Include guard */
#define WEBSERVER_H_

#define WEBSERVER_PORT 80 // HTTP server port
#define WEBSERVER_BUFFER_SIZE                                                  \
  1024 // Buffer size for HTTP requests and responses

extern QueueHandle_t trafficStateQueue; // Queue for traffic state data

/**
 * @brief Initializes and starts the web server task.
 *
 * @param port Port number for the web server (default is WEBSERVER_PORT).
 */
void start_webserver(uint16_t port);

/**
 * @brief Processes a single client request.
 *
 * @param client_socket Socket file descriptor for the connected client.
 */
void handle_client(int client_socket);

/**
 * @brief Fetches the latest traffic state data and formats it as JSON.
 *
 * @param buffer Buffer to store the JSON response.
 * @param buffer_size Size of the buffer.
 * @return int Length of the JSON response written to the buffer.
 */
int get_traffic_state_json(char *buffer, size_t buffer_size);

#endif // _WEBSERVER_H_
