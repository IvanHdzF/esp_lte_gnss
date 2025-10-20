idf_component_register(SRCS "gnss.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_gpio c_uart)
