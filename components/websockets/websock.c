#include "websock.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "icm20948.h"
#include <string.h>

static const char *TAG = "websocket";
static esp_websocket_client_handle_t client;

static void ws_evt(void *arg, esp_event_base_t base, int32_t id, void *data) {
    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:    ESP_LOGI(TAG, "WS connected"); break;
    case WEBSOCKET_EVENT_DISCONNECTED: ESP_LOGW(TAG, "WS disconnected"); break;
    case WEBSOCKET_EVENT_ERROR:        ESP_LOGE(TAG, "WS error"); break;
    default: break;
    }
}

void websocket_imu_task(void* arg)
{
    esp_websocket_client_config_t cfg = {
        .uri = "ws://192.168.137.194:8080/",
        // Keep these if your IDF has them; otherwise remove any unknown fields.
        .reconnect_timeout_ms = 5000,     // try every 5s
        .network_timeout_ms   = 8000,     // connect/read timeout
        .ping_interval_sec    = 15,       // send ping every 15s
        .pingpong_timeout_sec = 10,       // if no pong in 10s, reconnect
        .disable_auto_reconnect = false,
    };

    client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, ws_evt, NULL);

    ESP_ERROR_CHECK(esp_websocket_client_start(client));

    // Wait up to ~5s for first connection
    for (int i = 0; i < 50 && !esp_websocket_client_is_connected(client); ++i) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (1) {
        // If not connected, just sleep; auto-reconnect will kick in
        if (!esp_websocket_client_is_connected(client)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        imu_data_t data;
        if (get_imu_data(&data)) {
            char buf[192];
            int n = snprintf(buf, sizeof(buf),
                "{\"type\":\"imu\","
                "\"accel\":[%.3f,%.3f,%.3f],"
                "\"gyro\":[%.3f,%.3f,%.3f],"
                "\"roll\":%.3f,"
                "\"pitch\":%.3f,"
                "\"yaw\":%.3f}",
                data.ax, data.ay, data.az,
                data.gx, data.gy, data.gz,
                0.0f, 0.0f, 0.0f
            );

            if (n > 0 && n < (int)sizeof(buf)) {
                int sent = esp_websocket_client_send_text(client, buf, n, pdMS_TO_TICKS(1000));
                if (sent < 0) ESP_LOGE(TAG, "Send failed");
            } else {
                ESP_LOGE(TAG, "JSON too long or snprintf error (n=%d)", n);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
