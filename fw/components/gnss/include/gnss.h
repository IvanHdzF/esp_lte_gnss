#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>

typedef void (*gnss_callback_t)(const void* data, uint16_t len);

void gnss_set_callback(gnss_callback_t cb);
void gnss_init(void);
void gnss_deinit(void);

#endif // GNSS_H