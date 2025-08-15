#include <stdio.h>
#include <math.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "icm20948_regs.h"
#include "icm20948.h"

#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_SCL_IO      22
#define I2C_MASTER_SDA_IO      21
#define I2C_MASTER_FREQ_HZ     400000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define ICM20948_ADDR          0x69
#define REG_BANK_SEL           0x7F
#define PWR_MGMT_1             0x06
#define ACCEL_XOUT_H           0x2D
#define GYRO_CONFIG_1          0x01
#define ACCEL_CONFIG           0x14

static const char *TAG = "ICM20948";

const float dps_to_rads = (float)(M_PI / 180.0);

static imu_data_t imu_data;
static SemaphoreHandle_t imu_data_mutex;

static esp_err_t i2c_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM20948_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM20948_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_write_byte(uint8_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM20948_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void select_bank(uint8_t bank)
{
    i2c_write_byte(REG_BANK_SEL, bank);
}

esp_err_t imu_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) return ret;
    
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                           I2C_MASTER_RX_BUF_DISABLE,
                           I2C_MASTER_TX_BUF_DISABLE, 0);
    if (ret != ESP_OK) return ret;

    select_bank(0x00);
    ret = i2c_write_byte(PWR_MGMT_1, 0x01);
    if (ret != ESP_OK) return ret;
    
    vTaskDelay(pdMS_TO_TICKS(100));

    select_bank(0x20);
    ret = i2c_write_byte(GYRO_CONFIG_1, 0x00);
    if (ret != ESP_OK) return ret;
    
    ret = i2c_write_byte(ACCEL_CONFIG, 0x00);
    if (ret != ESP_OK) return ret;
    
    select_bank(0x00);

    imu_data_mutex = xSemaphoreCreateMutex();
    if (imu_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create imu_data_mutex");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}


void imu_task(void *arg)
{
    while (1) {
        uint8_t raw_data[12] = {0};

        if (i2c_read_bytes(ACCEL_XOUT_H, raw_data, 12) == ESP_OK) {
            int16_t ax = (raw_data[0] << 8) | raw_data[1];
            int16_t ay = (raw_data[2] << 8) | raw_data[3];
            int16_t az = (raw_data[4] << 8) | raw_data[5];
            int16_t gx = (raw_data[6] << 8) | raw_data[7];
            int16_t gy = (raw_data[8] << 8) | raw_data[9];
            int16_t gz = (raw_data[10] << 8) | raw_data[11];

            // convert raw to physical units 

            imu_data_t temp = {
                .ax = ax / 16384.0f * 9.80665f,
                .ay = ay / 16384.0f * 9.80665f,
                .az = az / 16384.0f * 9.80665f,
                .gx = gx / 131.0f * dps_to_rads,
                .gy = gy / 131.0f * dps_to_rads,
                .gz = gz / 131.0f * dps_to_rads
            };

            if (xSemaphoreTake(imu_data_mutex, pdMS_TO_TICKS(10))) {
                imu_data = temp;
                xSemaphoreGive(imu_data_mutex);
            }


            ESP_LOGI(TAG, "\nAccel [X: %.2fm/s^2, Y: %.2fm/s^2, Z: %.2fm/s^2] \nGyro [X: %.2f°/s, Y: %.2f°/s, Z: %.2f°/s]\n",
                     temp.ax, temp.ay, temp.az, temp.gx, temp.gy, temp.gz);
        } else {
            ESP_LOGE(TAG, "Failed to read sensor data");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool get_imu_data(imu_data_t *out)
{
    if (xSemaphoreTake(imu_data_mutex, pdMS_TO_TICKS(10))) {
        *out = imu_data;
        xSemaphoreGive(imu_data_mutex);
        return true;
    }
    return false;
}
