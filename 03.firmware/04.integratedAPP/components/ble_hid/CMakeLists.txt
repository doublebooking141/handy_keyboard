idf_component_register(
    SRCS
        "src/ble_hid_init.c"
        "src/flick_input.c"
    INCLUDE_DIRS "include"
    REQUIRES
        bt
        esp_hosted
        common
        nvs_flash
    PRIV_REQUIRES
        esp_event
        log
        esp_timer
        lvgl
)
