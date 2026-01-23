# Audio Player component
# WAV file playback via ES8311 codec

idf_component_register(
    SRCS
        "src/audio_player.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        freertos
        esp_common
        log
    PRIV_REQUIRES
        bsp_jc4880p443c
)
