#include <stdio.h>
#include "c_uart.h"

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// LilyGO T-SIM7000G Pinout
#define UART_BAUD           115200
#define UART_TX              27
#define UART_RX              26
#define UART_RTS_PIN   UART_PIN_NO_CHANGE
#define UART_CTS_PIN   UART_PIN_NO_CHANGE


const uart_port_t uart_num = UART_NUM_2;

static const char *TAG = "custom_uart";


void c_uart_init(void)
{
    ESP_LOGI(TAG, "Initializing UART...");
    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TX, UART_RX, UART_RTS_PIN, UART_CTS_PIN));

    ESP_LOGI(TAG, "UART initialized on port %d", uart_num);
}

void c_uart_deinit(void)
{
    // Uninstall UART driver
    ESP_LOGI(TAG, "Deinitializing UART...");
    ESP_ERROR_CHECK(uart_driver_delete(uart_num));
}

int c_uart_send_data(const void* data, size_t len)
{
    ESP_LOGD(TAG, "Sending %d bytes over UART...", len);
    return uart_write_bytes(uart_num, data, len);
}

int c_uart_read_data(void* buffer, size_t len, uint32_t timeout_ms)
{
    ESP_LOGD(TAG, "Reading %d bytes from UART...", len);
    return uart_read_bytes(uart_num, buffer, len, pdMS_TO_TICKS(timeout_ms));
}
