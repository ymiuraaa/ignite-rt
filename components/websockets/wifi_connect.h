#pragma once

#include "esp_err.h"
#include "esp_wifi_types.h"

// WiFi connection status bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT     BIT1
#define WIFI_MAXIMUM_RETRY 5

/**
 * @brief Initialize WiFi connection in station mode
 * 
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t wifi_connection_init(const char* ssid, const char* password);

/**
 * @brief Wait for WiFi connection to establish and IP to be assigned
 * 
 * @param timeout_ms Timeout in milliseconds (use 0 for no timeout)
 * @return esp_err_t ESP_OK if connected, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t wifi_connection_wait_for_ip(uint32_t timeout_ms);

/**
 * @brief Get the current IP address as string
 * 
 * @param ip_str Buffer to store IP address string
 * @param max_len Maximum length of buffer
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_connection_get_ip(char* ip_str, size_t max_len);