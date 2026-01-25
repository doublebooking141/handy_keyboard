/**
 * @file wol.c
 * @brief Wake-on-LAN magic packet sender
 */

#include "wol.h"
#include "network.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "WOL";

// NVS namespace and keys
#define NVS_NAMESPACE "network"
#define NVS_KEY_WOL_MAC "wol_mac"

// WOL constants
#define WOL_PORT 9
#define WOL_MAGIC_HEADER_LEN 6
#define WOL_MAC_REPEAT 16
#define WOL_PACKET_LEN (WOL_MAGIC_HEADER_LEN + (6 * WOL_MAC_REPEAT))

// State
static bool g_initialized = false;
static uint8_t g_default_mac[6] = {0};

/**
 * @brief Build WOL magic packet
 */
static void build_magic_packet(uint8_t *packet, const uint8_t mac[6])
{
    // Header: 6 bytes of 0xFF
    memset(packet, 0xFF, WOL_MAGIC_HEADER_LEN);

    // Repeat MAC address 16 times
    for (int i = 0; i < WOL_MAC_REPEAT; i++) {
        memcpy(packet + WOL_MAGIC_HEADER_LEN + (i * 6), mac, 6);
    }
}

esp_err_t wol_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WOL subsystem...");

    // Load default MAC from NVS
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t len = 6;
        if (nvs_get_blob(nvs_handle, NVS_KEY_WOL_MAC, g_default_mac, &len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded default MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     g_default_mac[0], g_default_mac[1], g_default_mac[2],
                     g_default_mac[3], g_default_mac[4], g_default_mac[5]);
        }
        nvs_close(nvs_handle);
    }

#ifdef CONFIG_HANDY_WOL_DEFAULT_MAC
    // If no MAC saved and Kconfig has default, parse it
    if (g_default_mac[0] == 0 && g_default_mac[1] == 0 && g_default_mac[2] == 0 &&
        g_default_mac[3] == 0 && g_default_mac[4] == 0 && g_default_mac[5] == 0) {
        if (wol_parse_mac(CONFIG_HANDY_WOL_DEFAULT_MAC, g_default_mac) == ESP_OK) {
            ESP_LOGI(TAG, "Using Kconfig default MAC: %s", CONFIG_HANDY_WOL_DEFAULT_MAC);
        }
    }
#endif

    g_initialized = true;
    ESP_LOGI(TAG, "WOL subsystem initialized");

    return ESP_OK;
}

esp_err_t wol_deinit(void)
{
    if (!g_initialized) {
        return ESP_OK;
    }

    g_initialized = false;
    ESP_LOGI(TAG, "WOL subsystem deinitialized");

    return ESP_OK;
}

wol_result_t wol_send(const uint8_t mac[6])
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "WOL not initialized");
        return WOL_RESULT_SEND_FAILED;
    }

    if (!network_is_connected()) {
        ESP_LOGE(TAG, "Network not connected");
        return WOL_RESULT_NOT_CONNECTED;
    }

    // Validate MAC (at least one non-zero byte)
    bool valid = false;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0) {
            valid = true;
            break;
        }
    }
    if (!valid) {
        ESP_LOGE(TAG, "Invalid MAC address (all zeros)");
        return WOL_RESULT_INVALID_MAC;
    }

    ESP_LOGI(TAG, "Sending WOL packet to %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Build magic packet
    uint8_t packet[WOL_PACKET_LEN];
    build_magic_packet(packet, mac);

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return WOL_RESULT_SEND_FAILED;
    }

    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE(TAG, "Failed to set SO_BROADCAST: %d", errno);
        close(sock);
        return WOL_RESULT_SEND_FAILED;
    }

    // Setup destination address (broadcast)
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(WOL_PORT);
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // Send magic packet
    int sent = sendto(sock, packet, WOL_PACKET_LEN, 0,
                      (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    close(sock);

    if (sent != WOL_PACKET_LEN) {
        ESP_LOGE(TAG, "Failed to send packet: sent=%d, expected=%d", sent, WOL_PACKET_LEN);
        return WOL_RESULT_SEND_FAILED;
    }

    ESP_LOGI(TAG, "WOL packet sent successfully (%d bytes)", sent);
    return WOL_RESULT_OK;
}

wol_result_t wol_send_default(void)
{
    if (!g_initialized) {
        return WOL_RESULT_SEND_FAILED;
    }

    // Check if default MAC is set
    bool has_mac = false;
    for (int i = 0; i < 6; i++) {
        if (g_default_mac[i] != 0) {
            has_mac = true;
            break;
        }
    }

    if (!has_mac) {
        ESP_LOGE(TAG, "No default MAC address configured");
        return WOL_RESULT_INVALID_MAC;
    }

    return wol_send(g_default_mac);
}

esp_err_t wol_parse_mac(const char *mac_str, uint8_t mac_bytes[6])
{
    if (!mac_str || !mac_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    unsigned int bytes[6];
    int count;

    // Try colon format first (AA:BB:CC:DD:EE:FF)
    count = sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                   &bytes[0], &bytes[1], &bytes[2],
                   &bytes[3], &bytes[4], &bytes[5]);

    if (count != 6) {
        // Try dash format (AA-BB-CC-DD-EE-FF)
        count = sscanf(mac_str, "%02x-%02x-%02x-%02x-%02x-%02x",
                       &bytes[0], &bytes[1], &bytes[2],
                       &bytes[3], &bytes[4], &bytes[5]);
    }

    if (count != 6) {
        ESP_LOGE(TAG, "Invalid MAC format: %s", mac_str);
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < 6; i++) {
        mac_bytes[i] = (uint8_t)bytes[i];
    }

    return ESP_OK;
}

esp_err_t wol_get_default_mac(uint8_t mac_bytes[6])
{
    if (!mac_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(mac_bytes, g_default_mac, 6);
    return ESP_OK;
}

esp_err_t wol_set_default_mac(const uint8_t mac_bytes[6])
{
    if (!mac_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, NVS_KEY_WOL_MAC, mac_bytes, 6);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        memcpy(g_default_mac, mac_bytes, 6);
        ESP_LOGI(TAG, "Default MAC set to: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac_bytes[0], mac_bytes[1], mac_bytes[2],
                 mac_bytes[3], mac_bytes[4], mac_bytes[5]);
    }

    return ret;
}

esp_err_t wol_get_default_mac_str(char *mac_str, size_t len)
{
    if (!mac_str || len < 18) {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(mac_str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             g_default_mac[0], g_default_mac[1], g_default_mac[2],
             g_default_mac[3], g_default_mac[4], g_default_mac[5]);

    return ESP_OK;
}
