idf_component_register(
    SRCS "sensor_handler.c"
    INCLUDE_DIRS "."
    REQUIRES adc spi i2c
)