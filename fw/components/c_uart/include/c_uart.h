#ifndef C_UART_H
#define C_UART_H

void c_uart_init(void);
void c_uart_deinit(void);
int c_uart_send_data(const void* data, size_t len);
int c_uart_read_data(void* buffer, size_t len, uint32_t timeout_ms);


#endif // C_UART_H