# SquareLine Studio generated UI component
# Adapted for ESP-IDF

# Collect all source files
file(GLOB_RECURSE UI_SOURCES
    "*.c"
)

idf_component_register(
    SRCS ${UI_SOURCES}
    INCLUDE_DIRS "." "screens" "fonts" "images" "components"
    REQUIRES lvgl ble_hid sdcard esp_driver_jpeg mjpeg_player alarm common
    PRIV_REQUIRES bsp_jc4880p443c
)
