/**
 * @file ntp.c
 * @brief NTP time synchronization with RTC update
 */

#include "ntp.h"
#include "network.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>
#include <sys/time.h>

static const char *TAG = "NTP";

// Default NTP sync timeout (30 seconds)
#define DEFAULT_NTP_TIMEOUT_MS 30000

// NVS namespace and keys
#define NVS_NAMESPACE "network"
#define NVS_KEY_NTP_SERVER "ntp_server"

// NTP server hostnames
static const char *NTP_SERVERS[] = {
    "ntp.nict.jp",      // NICT Japan
    "time.google.com",  // Google
};

// Server display names
static const char *NTP_SERVER_NAMES[] = {
    "NICT (ntp.nict.jp)",
    "Google (time.google.com)",
};

// State
static bool g_initialized = false;
static ntp_status_t g_status = NTP_STATUS_IDLE;
static ds3231m_handle_t *g_rtc_handle = NULL;
static ntp_sync_cb_t g_sync_callback = NULL;
static ntp_server_t g_current_server = NTP_SERVER_NICT;

// Timeout timer
static TimerHandle_t g_timeout_timer = NULL;
static uint32_t g_timeout_ms = DEFAULT_NTP_TIMEOUT_MS;

/**
 * @brief NTP sync timeout callback
 */
static void ntp_timeout_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    ESP_LOGE(TAG, "NTP sync timeout after %lu ms", (unsigned long)g_timeout_ms);

    // Stop SNTP operation
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }

    g_status = NTP_STATUS_FAILED;

    if (g_sync_callback) {
        g_sync_callback(g_status);
    }
}

/**
 * @brief SNTP time sync notification callback
 */
static void ntp_sync_notification_cb(struct timeval *tv)
{
    // Stop timeout timer on successful sync
    if (g_timeout_timer) {
        xTimerStop(g_timeout_timer, 0);
    }

    ESP_LOGI(TAG, "NTP sync completed");

    // Get synced time
    time_t now = tv->tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Synced time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Update DS3231M external RTC
    if (g_rtc_handle && g_rtc_handle->dev_handle) {
        esp_err_t ret = ds3231m_set_time(g_rtc_handle, &timeinfo);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "DS3231M RTC updated successfully");
        } else {
            ESP_LOGW(TAG, "Failed to update DS3231M: %s", esp_err_to_name(ret));
        }
    }

    g_status = NTP_STATUS_SYNCED;

    if (g_sync_callback) {
        g_sync_callback(g_status);
    }
}

esp_err_t ntp_init(ds3231m_handle_t *rtc_handle)
{
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing NTP subsystem...");

    g_rtc_handle = rtc_handle;

    // Load default server from NVS
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK) {
        uint8_t server;
        if (nvs_get_u8(nvs_handle, NVS_KEY_NTP_SERVER, &server) == ESP_OK) {
            if (server < NTP_SERVER_MAX) {
                g_current_server = (ntp_server_t)server;
            }
        }
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Default NTP server: %s", NTP_SERVER_NAMES[g_current_server]);

    // Set timezone to JST (UTC+9)
    setenv("TZ", "JST-9", 1);
    tzset();

    g_initialized = true;
    ESP_LOGI(TAG, "NTP subsystem initialized");

    return ESP_OK;
}

esp_err_t ntp_deinit(void)
{
    if (!g_initialized) {
        return ESP_OK;
    }

    // Stop and delete timeout timer
    if (g_timeout_timer) {
        xTimerStop(g_timeout_timer, 0);
        xTimerDelete(g_timeout_timer, 0);
        g_timeout_timer = NULL;
    }

    esp_sntp_stop();
    g_initialized = false;
    g_status = NTP_STATUS_IDLE;
    ESP_LOGI(TAG, "NTP subsystem deinitialized");

    return ESP_OK;
}

esp_err_t ntp_sync(ntp_server_t server)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "NTP not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!network_is_connected()) {
        ESP_LOGE(TAG, "Network not connected");
        g_status = NTP_STATUS_FAILED;
        if (g_sync_callback) {
            g_sync_callback(g_status);
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (server >= NTP_SERVER_MAX) {
        server = NTP_SERVER_NICT;
    }

    ESP_LOGI(TAG, "Starting NTP sync with %s", NTP_SERVERS[server]);
    g_status = NTP_STATUS_SYNCING;

    if (g_sync_callback) {
        g_sync_callback(g_status);
    }

    // Stop any existing SNTP operation
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }

    // Configure SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVERS[server]);
    esp_sntp_set_time_sync_notification_cb(ntp_sync_notification_cb);

    // Start SNTP (non-blocking)
    // Sync completion will be notified via ntp_sync_notification_cb
    esp_sntp_init();

    // Start timeout timer
    if (!g_timeout_timer) {
        g_timeout_timer = xTimerCreate("ntp_timeout",
                                        pdMS_TO_TICKS(g_timeout_ms),
                                        pdFALSE,  // One-shot timer
                                        NULL,
                                        ntp_timeout_cb);
        if (!g_timeout_timer) {
            ESP_LOGE(TAG, "Failed to create NTP timeout timer");
        }
    } else {
        // Update period if timeout was changed
        xTimerChangePeriod(g_timeout_timer, pdMS_TO_TICKS(g_timeout_ms), 0);
    }

    if (g_timeout_timer) {
        xTimerStart(g_timeout_timer, 0);
        ESP_LOGI(TAG, "NTP timeout timer started (%lu ms)", (unsigned long)g_timeout_ms);
    }

    ESP_LOGI(TAG, "NTP sync started (async)");
    return ESP_OK;
}

ntp_status_t ntp_get_status(void)
{
    return g_status;
}

esp_err_t ntp_set_time_manual(const struct tm *time)
{
    if (!time) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting time manually: %04d-%02d-%02d %02d:%02d:%02d",
             time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
             time->tm_hour, time->tm_min, time->tm_sec);

    // Set ESP32 internal RTC
    struct tm time_copy = *time;
    time_t now = mktime(&time_copy);
    struct timeval tv = {
        .tv_sec = now,
        .tv_usec = 0
    };

    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "Failed to set ESP32 internal RTC");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ESP32 internal RTC updated");

    // Set DS3231M external RTC
    if (g_rtc_handle && g_rtc_handle->dev_handle) {
        esp_err_t ret = ds3231m_set_time(g_rtc_handle, time);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "DS3231M RTC updated successfully");
        } else {
            ESP_LOGW(TAG, "Failed to update DS3231M: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t ntp_set_default_server(ntp_server_t server)
{
    if (server >= NTP_SERVER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_u8(nvs_handle, NVS_KEY_NTP_SERVER, (uint8_t)server);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (ret == ESP_OK) {
        g_current_server = server;
        ESP_LOGI(TAG, "Default NTP server set to: %s", NTP_SERVER_NAMES[server]);
    }

    return ret;
}

ntp_server_t ntp_get_default_server(void)
{
    return g_current_server;
}

const char *ntp_get_server_name(ntp_server_t server)
{
    if (server >= NTP_SERVER_MAX) {
        return "Unknown";
    }
    return NTP_SERVER_NAMES[server];
}

void ntp_register_callback(ntp_sync_cb_t cb)
{
    g_sync_callback = cb;
}

esp_err_t ntp_set_timeout(uint32_t timeout_ms)
{
    if (timeout_ms < 1000 || timeout_ms > 300000) {
        ESP_LOGE(TAG, "Invalid timeout: %lu ms (valid range: 1000-300000)", (unsigned long)timeout_ms);
        return ESP_ERR_INVALID_ARG;
    }

    g_timeout_ms = timeout_ms;
    ESP_LOGI(TAG, "NTP timeout set to %lu ms", (unsigned long)timeout_ms);
    return ESP_OK;
}

uint32_t ntp_get_timeout(void)
{
    return g_timeout_ms;
}
