idf_component_register(SRCS "main.c" "adc_wrapper.c" "storage_handler.c"
                    PRIV_REQUIRES spi_flash esp_adc esp_driver_gpio nvs_flash
                    INCLUDE_DIRS "./")
