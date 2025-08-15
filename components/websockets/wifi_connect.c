#include "wifi_connect.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "secrets.h"
#include <string.h>

static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_event_group;
static esp_ip4_addr_t s_ip_addr;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    const wifi_event_sta_disconnected_t *e = (const wifi_event_sta_disconnected_t*)event_data;
    ESP_LOGE(TAG, "DISCONNECTED: reason=%d", e ? e->reason : -1);
    // Common hints
    // 2=AUTH_EXPIRE, 4=ASSOC_EXPIRE, 15=4WAY_HANDSHAKE_TIMEOUT,
    // 201=NO_AP_FOUND, 202=AUTH_FAIL, 203=ASSOC_FAIL, 205=HANDSHAKE_TIMEOUT
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {
        ESP_LOGI(TAG, "Retry connection to AP... (%d/%d)", s_retry_num + 1, WIFI_MAXIMUM_RETRY);
        esp_wifi_connect();
        s_retry_num++;
    } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGE(TAG, "Failed after %d attempts", WIFI_MAXIMUM_RETRY);
    }
} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&s_ip_addr));
    }
}

esp_err_t wifi_connection_init(const char* ssid, const char* password)
{
    if (!ssid || !password) {
        ESP_LOGE(TAG, "Invalid SSID or password");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting WiFi initialization");

    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Create default WiFi station interface
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        ESP_LOGE(TAG, "Failed to create station interface");
        vEventGroupDelete(s_wifi_event_group);
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register event handlers
    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &event_handler,
                                            NULL,
                                            NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(IP_EVENT,
                                            IP_EVENT_STA_GOT_IP,
                                            &event_handler,
                                            NULL,
                                            NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure WiFi station
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ESP32-test",
            .password = "FLeepy77?",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };


ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));


    strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    // Configure WiFi mode and start
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi initialization completed");
    return ESP_OK;
}

esp_err_t wifi_connection_wait_for_ip(uint32_t timeout_ms)
{
    if (!s_wifi_event_group) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP");
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Connection timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_connection_get_ip(char* ip_str, size_t max_len)
{
    if (!ip_str || max_len < 16) {
        ESP_LOGE(TAG, "Invalid buffer for IP address");
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(ip_str, max_len, IPSTR, IP2STR(&s_ip_addr));
    return ESP_OK;
}