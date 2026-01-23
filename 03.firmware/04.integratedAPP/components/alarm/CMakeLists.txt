# Alarm component
# Multiple alarm management with NVS storage

idf_component_register(
    SRCS
        "src/alarm.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        freertos
        esp_common
        log
        nvs_flash
        audio_player
)
