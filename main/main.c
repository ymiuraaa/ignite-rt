#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icm20948.h"
#include "wifi_connect.h"
#include "sdkconfig.h"
#include "websock.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "secrets.h"

#define WIFI_TIMEOUT_MS    30000


static const char *TAG = "main";

void app_main(void)
{
    // Set log level to info for all components
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Starting initialization sequence");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized");

    // Initialize TCP/IP adapter first
    ESP_LOGI(TAG, "Initializing TCP/IP adapter...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "TCP/IP adapter initialized");

    // Initialize IMU
    ESP_LOGI(TAG, "Initializing IMU...");
    ret = imu_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IMU initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "IMU initialized successfully");

    // Initialize WiFi with proper error handling
    ESP_LOGI(TAG, "Starting WiFi initialization...");
    ret = wifi_connection_init(WIFI_SSID, WIFI_PASSWORD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "WiFi initialized, waiting for connection...");

    // Wait for WiFi connection
    ret = wifi_connection_wait_for_ip(WIFI_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed: %s", esp_err_to_name(ret));
        return;
    }

    char ip[16];
    ret = wifi_connection_get_ip(ip, sizeof(ip));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Connected with IP: %s", ip);
    }

    // Create tasks only after WiFi is connected
    BaseType_t task_ret = xTaskCreate(imu_task, "imu_task", 4096, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create IMU task");
        return;
    }
    ESP_LOGI(TAG, "IMU task created");

    task_ret = xTaskCreate(websocket_imu_task, "websocket_task", 8192, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create websocket task");
        return;
    }
    ESP_LOGI(TAG, "Websocket task created");

    ESP_LOGI(TAG, "Initialization complete");
}