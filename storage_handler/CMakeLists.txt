idf_component_register(
    SRCS "storage_handler.c"
    PRIV_REQUIRES nvs_flash
    INCLUDE_DIRS "."
)