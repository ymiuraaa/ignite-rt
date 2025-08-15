#include "esp_websocket_client.h"
#include <stddef.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_connect_start(const char *ssid, const char *pass);
void      wifi_wait_for_ip(void);
void      wifi_get_ip(char *ip, size_t maxlen);
void      websocket_imu_task(void *arg);


#ifdef __cplusplus
}
#endif

