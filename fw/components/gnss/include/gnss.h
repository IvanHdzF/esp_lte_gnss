#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>


/* Commands */
#define AT "AT"
#define AT_INIT_GNSS  "AT+CGNSPWR=1"
#define AT_DEINIT_GNSS  "AT+CGNSPWR=0"
#define AT_GET_GNS_INF "AT+CGNSINF"

typedef struct {
    uint8_t  gnss_run_status;       // 0 = GNSS off, 1 = GNSS on
    uint8_t  fix_status;            // 0 = No fix, 1 = Fix
    char     utc_datetime[24];      // yyyyMMddhhmmss.sss (null-terminated)
    double   latitude;              // [-90.000000, 90.000000]
    double   longitude;             // [-180.000000, 180.000000]
    float    altitude_msl;          // meters
    float    speed_kmh;             // km/h
    float    course_deg;            // degrees
    uint8_t  fix_mode;              // 0 = No fix, 1 = 2D, 2 = 3D
    float    hdop;                  // Horizontal Dilution of Precision
    float    pdop;                  // Position DOP
    float    vdop;                  // Vertical DOP
    uint8_t  sats_in_view;          // Total satellites in view
    uint8_t  sats_used_gps;         // GPS satellites used
    uint8_t  sats_used_glonass;     // GLONASS satellites used
    uint8_t  cn0_max;               // Max C/N0 (carrier-to-noise ratio)
    float    hpa;                   // Horizontal Position Accuracy (m)
    float    vpa;                   // Vertical Position Accuracy (m)
} sim7000_gnss_info_t;


typedef void (*gnss_callback_t)(const void* data, uint16_t len);

void gnss_set_callback(gnss_callback_t cb);
void gnss_init(void);
void gnss_deinit(void);

#endif // GNSS_H