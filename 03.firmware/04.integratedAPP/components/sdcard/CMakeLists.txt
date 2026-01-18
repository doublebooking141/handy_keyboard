idf_component_register(
    SRCS "src/sdcard.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
    PRIV_REQUIRES bsp_jc4880p443c
)
