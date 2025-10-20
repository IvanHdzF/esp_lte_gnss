#include "gnss.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "c_uart.h"

#define SIMCOM_PIN_DTR             25
#define SIMCOM_PWR_PIN             4
#define SIMCOM_BLUE_LED            12

#define CMD_TERMINATOR "\r\n"
#define UART_RX_TIMEOUT_MS        500

#define SIMCOM_RSP_MAX_LENGTH 250

#define POLLING_RATE_MS        1000

static const char *TAG = "esp_gnss";

// Callback type and setter
static gnss_callback_t gnss_callback = NULL;

void gnss_set_callback(gnss_callback_t cb)
{
    gnss_callback = cb;
}

// Task handle
static TaskHandle_t gnss_task_handle = NULL;


int send_cmd(const char *cmd, size_t len, char *response, size_t rsp_max_len, uint32_t timeout_ms)
{
    char tx_buf[128];
    if (len + sizeof(CMD_TERMINATOR) >= sizeof(tx_buf)) {
        ESP_LOGE(TAG, "Command too long");
        return -1;
    }

    // Build full command string
    memcpy(tx_buf, cmd, len);
    memcpy(tx_buf + len, CMD_TERMINATOR, sizeof(CMD_TERMINATOR) - 1);
    size_t total_len = len + sizeof(CMD_TERMINATOR) - 1;

    int sent_chars = c_uart_send_data(tx_buf, total_len);
    ESP_LOGI(TAG, "Sent %d characters\nCMD: %s", sent_chars, tx_buf);

    int read_chars = c_uart_read_data(response, rsp_max_len, timeout_ms);
    ESP_LOGI(TAG, "Read %d characters\nResponse:\n%s", read_chars, response);

    return read_chars;
}


static void simcom_pwr_up(void)
{
    // Power on the SIMCOM module
    ESP_LOGI(TAG, "Powering up SIMCOM module...");
    gpio_set_level(SIMCOM_PWR_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(SIMCOM_PWR_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for module to power up
    ESP_LOGI(TAG, "SIMCOM module initialized");
}

// static void simcom_pwr_down(void)
// {
//     // Power off the SIMCOM module
//     gpio_set_level(SIMCOM_PWR_PIN, 1);
//     vTaskDelay(pdMS_TO_TICKS(1000));
//     gpio_set_level(SIMCOM_PWR_PIN, 0);
//     vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for module to power down
//     ESP_LOGI(TAG, "SIMCOM module powered down");
// }

static void verify_sim7000_serial_com(void)
{
    char rsp_buf[SIMCOM_RSP_MAX_LENGTH];
    ESP_LOGI(TAG, "Sending AT CMD...");
    uint8_t read_chars = send_cmd(AT, sizeof(AT), rsp_buf, SIMCOM_RSP_MAX_LENGTH, UART_RX_TIMEOUT_MS);

    /* Sanitize response buffer */
    rsp_buf[read_chars] = '\0';
    ESP_LOGI(TAG, "Sent AT\nReceived data:\n%s", rsp_buf);

    if (strncmp("OK", rsp_buf, sizeof("OK") != 0))
    {
        ESP_LOGE(TAG, "ERROR! OK Command not received from SIMCOM, verification failed...");
    }
    else
    {
        ESP_LOGI(TAG, "Successfully confirmed communication with SIMCOM!...OK!");
    }

}

static void gnss_pwr_up(void)
{
    char rsp_buf[SIMCOM_RSP_MAX_LENGTH];
    ESP_LOGI(TAG, "Sending AT CMD...");
    uint8_t read_chars = send_cmd(AT_INIT_GNSS, sizeof(AT_INIT_GNSS), rsp_buf, SIMCOM_RSP_MAX_LENGTH, UART_RX_TIMEOUT_MS);

    /* Sanitize response buffer */
    rsp_buf[read_chars] = '\0';
    ESP_LOGI(TAG, "Sent %s \nReceived data:\n%s", AT_INIT_GNSS, rsp_buf);

    if (strncmp("OK", rsp_buf, sizeof("OK") != 0))
    {
        ESP_LOGE(TAG, "ERROR! OK Command not received from SIMCOM...");
    }
    else
    {
        ESP_LOGI(TAG, "Successfully initialized GNSS module!...OK!");
    }
}

static void read_gnss_data(sim7000_gnss_info_t* info)
{
    char rsp_buf[SIMCOM_RSP_MAX_LENGTH];
    int read_chars = send_cmd(AT_GET_GNS_INF, sizeof(AT_GET_GNS_INF),
                              rsp_buf, SIMCOM_RSP_MAX_LENGTH, UART_RX_TIMEOUT_MS);

    rsp_buf[read_chars] = '\0';
    ESP_LOGI(TAG, "Received data:\n%s", rsp_buf);

    // Find +CGNSINF line
    char *line = strstr(rsp_buf, "+CGNSINF:");
    if (!line) {
        ESP_LOGE(TAG, "No +CGNSINF response found!");
        return;
    }

    line += strlen("+CGNSINF:"); // skip prefix

    // Tokenize by commas
    char *token;
    char *saveptr;
    int field = 0;

    token = strtok_r(line, ",", &saveptr);
    while (token) {
        switch (field) {
            case 0: info->gnss_run_status = atoi(token); break;
            case 1: info->fix_status = atoi(token); break;
            case 2: strncpy(info->utc_datetime, token, sizeof(info->utc_datetime)-1);
                    info->utc_datetime[sizeof(info->utc_datetime)-1] = '\0';
                    break;
            case 3: info->latitude = atof(token); break;
            case 4: info->longitude = atof(token); break;
            case 5: info->altitude_msl = atof(token); break;
            case 6: info->speed_kmh = atof(token); break;
            case 7: info->course_deg = atof(token); break;
            case 8: info->fix_mode = atoi(token); break;
            case 10: info->hdop = atof(token); break;
            case 11: info->pdop = atof(token); break;
            case 12: info->vdop = atof(token); break;
            case 14: info->sats_in_view = atoi(token); break;
            case 15: info->sats_used_gps = atoi(token); break;
            case 16: info->sats_used_glonass = atoi(token); break;
            case 18: info->cn0_max = atoi(token); break;
            case 19: info->hpa = atof(token); break;
            case 20: info->vpa = atof(token); break;
            default: break; // skip unused/reserved fields
        }
        field++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    ESP_LOGI(TAG, "GNSS Parsed: fix=%u lat=%.6f lon=%.6f", info->fix_status, info->latitude, info->longitude);
}


// Polling task
static void gnss_polling_task(void *param)
{   
    uint8_t toggle_val = 0;
    while (1) {
        gpio_set_level(SIMCOM_BLUE_LED, toggle_val);

        sim7000_gnss_info_t gnss_info = {0};
        read_gnss_data(&gnss_info);

        if (gnss_info.gnss_run_status != 1)
        {
            ESP_LOGE(TAG, "ERROR! GNSS is not on RUN status!");
        }

        if (gnss_info.fix_status != 1)
        {
            ESP_LOGW(TAG, "WARNING! No GNSS Fix acquired yet!");
        }

        ESP_LOGI(TAG, "GNSS polled: fix=%d lat=%.6f lon=%.6f, speed=%.6f", gnss_info.fix_status, gnss_info.latitude, gnss_info.longitude, gnss_info.speed_kmh);

        if (gnss_callback) {
            gnss_callback(&gnss_info, sizeof(gnss_info));
        }

        toggle_val++;
        toggle_val &= 0x01U;

        vTaskDelay(pdMS_TO_TICKS(POLLING_RATE_MS)); // Poll every second
    }
}

void gnss_init(void)
{
    // Configure GPIOs for SIMCOM module control
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SIMCOM_PIN_DTR) | (1ULL << SIMCOM_PWR_PIN) | (1ULL << SIMCOM_BLUE_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    simcom_pwr_up();

    c_uart_init();

    verify_sim7000_serial_com();

    gnss_pwr_up();

    if (gnss_task_handle == NULL) {
        xTaskCreate(gnss_polling_task, "gnss_poll", 4096, NULL, 5, &gnss_task_handle);
        ESP_LOGI(TAG, "GNSS polling task started");
    }
}

void gnss_deinit(void)
{
    if (gnss_task_handle) {
        vTaskDelete(gnss_task_handle);
        gnss_task_handle = NULL;
        ESP_LOGI(TAG, "GNSS polling task stopped");
    }
}