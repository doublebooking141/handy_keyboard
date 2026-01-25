/**
 * @file network.c
 * @brief WiFi network connection management via ESP-Hosted
 */

#include "network.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_hosted.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "NETWORK";

// NVS namespace and keys
#define NVS_NAMESPACE "network"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASSWORD "wifi_pass"

// Event group bits
#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1

// State
static bool g_initialized = false;
static bool g_connect_requested = false;  // Track if connect was explicitly requested
static network_state_t g_state = NETWORK_STATE_DISCONNECTED;
static EventGroupHandle_t g_wifi_event_group = NULL;
static esp_netif_t *g_sta_netif = NULL;
static network_event_cb_t g_event_callback = NULL;
static int g_retry_count = 0;
static const int MAX_RETRY = 5;

// Current connection info
static char g_current_ssid[33] = {0};
static esp_netif_ip_info_t g_ip_info;

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi STA started");
        // Only connect if explicitly requested (via network_connect)
        // Don't auto-connect here as credentials may not be configured yet
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d", event->reason);

        // Only retry if connection was explicitly requested
        if (g_connect_requested && g_retry_count < MAX_RETRY) {
            g_retry_count++;
            ESP_LOGI(TAG, "Retrying connection (%d/%d)...", g_retry_count, MAX_RETRY);
            esp_wifi_connect();
            g_state = NETWORK_STATE_CONNECTING;
        } else if (g_connect_requested) {
            ESP_LOGE(TAG, "Connection failed after %d retries", MAX_RETRY);
            xEventGroupSetBits(g_wifi_event_group, WIFI_FAIL_BIT);
            g_state = NETWORK_STATE_ERROR;
            g_connect_requested = false;
        } else {
            g_state = NETWORK_STATE_DISCONNECTED;
        }

        if (g_event_callback) {
            g_event_callback(g_state);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "Connected to AP: %s", event->ssid);
        g_retry_count = 0;
        // Wait for IP before setting CONNECTED state
    }
}

/**
 * @brief IP event handler
 */
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        memcpy(&g_ip_info, &event->ip_info, sizeof(esp_netif_ip_info_t));

        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        g_state = NETWORK_STATE_CONNECTED;
        xEventGroupSetBits(g_wifi_event_group, WIFI_CONNECTED_BIT);

        if (g_event_callback) {
            g_event_callback(g_state);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGW(TAG, "Lost IP address");
        memset(&g_ip_info, 0, sizeof(g_ip_info));
    }
}

esp_err_t network_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing network subsystem...");

    // Verify ESP-Hosted coprocessor is connected
    // (BLE HID init should have already established this connection)
    esp_hosted_coprocessor_fwver_t fwver;
    if (esp_hosted_get_coprocessor_fwversion(&fwver) == ESP_OK) {
        ESP_LOGI(TAG, "ESP-Hosted coprocessor FW: %lu.%lu.%lu",
                 fwver.major1, fwver.minor1, fwver.patch1);
    } else {
        ESP_LOGW(TAG, "Could not get ESP-Hosted coprocessor version - WiFi may not work");
    }

    // Create event group
    g_wifi_event_group = xEventGroupCreate();
    if (!g_wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop (may already exist from other components)
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create default WiFi STA netif
    g_sta_netif = esp_netif_create_default_wifi_sta();
    if (!g_sta_netif) {
        ESP_LOGE(TAG, "Failed to create WiFi STA netif");
        return ESP_FAIL;
    }

    // Initialize WiFi with default config
    // Note: When using ESP-Hosted, esp_wifi APIs are remapped to esp_wifi_remote
    // which communicates with the ESP32-C6 coprocessor
    ESP_LOGI(TAG, "Initializing WiFi (via ESP-Hosted)...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "WiFi init successful");

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_LOST_IP, &ip_event_handler, NULL, NULL));

    // Set WiFi mode to STA
    ESP_LOGI(TAG, "Setting WiFi mode to STA...");
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi set mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start WiFi
    ESP_LOGI(TAG, "Starting WiFi...");
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "WiFi started successfully");

    g_initialized = true;
    g_state = NETWORK_STATE_DISCONNECTED;
    ESP_LOGI(TAG, "Network subsystem initialized");

    return ESP_OK;
}

esp_err_t network_deinit(void)
{
    if (!g_initialized) {
        return ESP_OK;
    }

    esp_wifi_stop();
    esp_wifi_deinit();

    if (g_sta_netif) {
        esp_netif_destroy(g_sta_netif);
        g_sta_netif = NULL;
    }

    if (g_wifi_event_group) {
        vEventGroupDelete(g_wifi_event_group);
        g_wifi_event_group = NULL;
    }

    g_initialized = false;
    g_state = NETWORK_STATE_DISCONNECTED;
    ESP_LOGI(TAG, "Network subsystem deinitialized");

    return ESP_OK;
}

esp_err_t network_connect(const char *ssid, const char *password)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "Network not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    // Store SSID for reference
    strncpy(g_current_ssid, ssid, sizeof(g_current_ssid) - 1);
    g_current_ssid[sizeof(g_current_ssid) - 1] = '\0';

    // Configure WiFi
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password && strlen(password) > 0) {
        strncpy((char *)wifi_config.sta.password, password,
                sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = password && strlen(password) > 0 ?
                                         WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Reset retry counter and mark connection as requested
    g_retry_count = 0;
    g_connect_requested = true;
    g_state = NETWORK_STATE_CONNECTING;

    // Clear event bits
    xEventGroupClearBits(g_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Start connection
    ESP_LOGI(TAG, "Starting WiFi connection to: %s", ssid);
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed: %s", esp_err_to_name(ret));
        g_state = NETWORK_STATE_ERROR;
        g_connect_requested = false;
        return ret;
    }
    ESP_LOGI(TAG, "WiFi connect initiated");

    if (g_event_callback) {
        g_event_callback(g_state);
    }

    return ESP_OK;
}

esp_err_t network_connect_saved(void)
{
    char ssid[33] = {0};
    char password[65] = {0};
    nvs_handle_t nvs_handle;

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No saved credentials found");
        return ESP_ERR_NOT_FOUND;
    }

    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);

    ret = nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &ssid_len);
    if (ret != ESP_OK) {
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    // Password is optional
    nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &pass_len);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Connecting with saved credentials: %s", ssid);
    return network_connect(ssid, password);
}

esp_err_t network_disconnect(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Disconnecting from WiFi");
    g_connect_requested = false;  // Prevent auto-reconnect
    g_retry_count = MAX_RETRY;
    esp_wifi_disconnect();
    g_state = NETWORK_STATE_DISCONNECTED;

    if (g_event_callback) {
        g_event_callback(g_state);
    }

    return ESP_OK;
}

bool network_is_connected(void)
{
    return g_state == NETWORK_STATE_CONNECTED;
}

network_state_t network_get_state(void)
{
    return g_state;
}

esp_err_t network_get_ip(char *ip_str, size_t len)
{
    if (!ip_str || len < 16) {
        return ESP_ERR_INVALID_ARG;
    }

    if (g_state != NETWORK_STATE_CONNECTED) {
        return ESP_ERR_INVALID_STATE;
    }

    snprintf(ip_str, len, IPSTR, IP2STR(&g_ip_info.ip));
    return ESP_OK;
}

esp_err_t network_save_credentials(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
    if (ret != ESP_OK) {
        nvs_close(nvs_handle);
        return ret;
    }

    if (password && strlen(password) > 0) {
        ret = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password);
        if (ret != ESP_OK) {
            nvs_close(nvs_handle);
            return ret;
        }
    } else {
        nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
    }

    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials saved for: %s", ssid);
    return ret;
}

esp_err_t network_clear_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    nvs_erase_key(nvs_handle, NVS_KEY_SSID);
    nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials cleared");
    return ret;
}

bool network_has_saved_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return false;
    }

    size_t len = 0;
    ret = nvs_get_str(nvs_handle, NVS_KEY_SSID, NULL, &len);
    nvs_close(nvs_handle);

    return (ret == ESP_OK && len > 0);
}

void network_register_callback(network_event_cb_t cb)
{
    g_event_callback = cb;
}
