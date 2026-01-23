idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        nvs_flash
        esp_driver_ppa
        common
        drivers
        bsp_jc4880p443c
        ui
        ble_hid
        sdcard
        alarm
)
