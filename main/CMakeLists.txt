idf_component_register(SRCS "main.c"
                    REQUIRES spi_flash esp_driver_gpio sensor_module
                    INCLUDE_DIRS "./")

