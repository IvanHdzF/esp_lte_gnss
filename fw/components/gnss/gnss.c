#include "gnss.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "esp_gnss";

// Callback type and setter
static gnss_callback_t gnss_callback = NULL;

void gnss_set_callback(gnss_callback_t cb)
{
    gnss_callback = cb;
}

// Task handle
static TaskHandle_t gnss_task_handle = NULL;

// Dummy GNSS data structure
typedef struct {
    int fix;
    double lat;
    double lon;
} gnss_data_t;

// Polling task
static void gnss_polling_task(void *param)
{
    while (1) {

        //TODO: Call SIM700G API here to retrieve data
        gnss_data_t data = { .fix = 1, .lat = 40.0, .lon = -3.0 }; // Dummy data

        ESP_LOGI(TAG, "GNSS polled: fix=%d lat=%.6f lon=%.6f", data.fix, data.lat, data.lon);

        if (gnss_callback) {
            gnss_callback(&data, sizeof(data));
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Poll every second
    }
}

void gnss_init(void)
{
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