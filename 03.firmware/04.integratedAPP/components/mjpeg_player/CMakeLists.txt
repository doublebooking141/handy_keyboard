idf_component_register(
    SRCS "src/mjpeg_player.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl esp_driver_jpeg freertos common
    PRIV_REQUIRES bsp_jc4880p443c
)
