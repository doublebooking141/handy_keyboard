# Network Component - WiFi, NTP, WOL

idf_component_register(
    SRCS "src/network.c" "src/ntp.c" "src/wol.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_common esp_wifi esp_netif lwip nvs_flash log drivers
    PRIV_REQUIRES esp_hosted
)
