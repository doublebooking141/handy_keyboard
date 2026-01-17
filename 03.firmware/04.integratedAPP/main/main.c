/**
 * @file main.c
 * @brief Handy Keyboard Firmware - Main Application
 *
 * ESP32-P4 based handy keyboard/controller with the following features:
 * - BT Keyboard (flick input + full keyboard)
 * - BT Mouse (touchpad with gesture support)
 * - Clock mode (digital/analog with SD card backgrounds)
 * - Wake-on-LAN (magic packet sender)
 * - Audio playback (MP3 alarm sounds)
 * - microSD operations (format, USB mass storage)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_timer.h"

// BSP (Board Support Package)
#include "bsp/jc4880p443c.h"
#include "lvgl.h"

// Common components
#include "keyboard_info.h"

// Drivers (for devices not covered by BSP)
#include "ds3231m.h"

// BLE HID
#include "ble_hid.h"

// SquareLine Studio generated UI
#include "ui.h"
#include "ui_navigation.h"
#include "ui_customization.h"

static const char *TAG = "HANDY_KEYBOARD";

// LVGL display handle
static lv_display_t *g_display = NULL;

// RTC handle (global for task access)
static ds3231m_handle_t g_rtc_handle;
static bool g_rtc_initialized = false;

/**
 * @brief Initialize NVS (Non-Volatile Storage)
 *
 * Required for WiFi, BLE, and settings storage
 */
static void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");
}

/**
 * @brief Initialize DS3231M RTC using BSP I2C handle
 */
static void rtc_init(void)
{
    i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
    if (i2c_handle == NULL) {
        ESP_LOGW(TAG, "I2C handle not available for RTC init");
        return;
    }

    esp_err_t ret = ds3231m_init(i2c_handle, &g_rtc_handle);
    if (ret == ESP_OK) {
        struct tm current_time;
        if (ds3231m_get_time(&g_rtc_handle, &current_time) == ESP_OK) {
            ESP_LOGI(TAG, "DS3231M RTC time: %04d-%02d-%02d %02d:%02d:%02d",
                     1900 + current_time.tm_year, current_time.tm_mon + 1,
                     current_time.tm_mday, current_time.tm_hour,
                     current_time.tm_min, current_time.tm_sec);

            // Set ESP32 internal RTC from DS3231M (read once at startup)
            // This avoids drift issues caused by frequent I2C queries
            struct timeval tv;
            tv.tv_sec = mktime(&current_time);
            tv.tv_usec = 0;
            if (settimeofday(&tv, NULL) == 0) {
                ESP_LOGI(TAG, "ESP32 internal RTC synchronized from DS3231M");
                g_rtc_initialized = true;
            } else {
                ESP_LOGW(TAG, "Failed to set ESP32 internal RTC");
            }
        } else {
            ESP_LOGW(TAG, "Failed to read time from DS3231M");
        }

        float temp;
        if (ds3231m_get_temperature(&g_rtc_handle, &temp) == ESP_OK) {
            ESP_LOGI(TAG, "RTC temperature: %.2f°C", temp);
        }
    } else {
        ESP_LOGW(TAG, "DS3231M RTC not found or failed to initialize");
    }
}

// Day of week names (Japanese abbreviation)
static const char *WEEKDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

/**
 * @brief RTC update task - updates UI with current time every second
 *
 * Uses ESP32 internal RTC (set from DS3231M at startup) to avoid
 * drift issues caused by frequent I2C queries to external RTC.
 */
static void rtc_update_task(void *pvParameters)
{
    struct tm time_info;
    char date_str[32];
    char time_str[8];
    time_t now;

    ESP_LOGI(TAG, "RTC update task started (using ESP32 internal RTC)");

    while (1) {
        if (g_rtc_initialized) {
            // Get time from ESP32 internal RTC (not DS3231M)
            time(&now);
            localtime_r(&now, &time_info);

            // Update clock hands (with LVGL lock)
            if (bsp_display_lock(100)) {
                ui_clock_update_hands(time_info.tm_hour, time_info.tm_min, time_info.tm_sec);

                // Format date: "YYYY/MM/DD (Day)"
                snprintf(date_str, sizeof(date_str), "%04d/%02d/%02d (%s)",
                         1900 + time_info.tm_year, time_info.tm_mon + 1,
                         time_info.tm_mday, WEEKDAY_NAMES[time_info.tm_wday]);

                // Format time: "HH:MM"
                snprintf(time_str, sizeof(time_str), "%02d:%02d",
                         time_info.tm_hour, time_info.tm_min);

                // Update header (battery % is placeholder for now)
                ui_header_update(date_str, time_str, 75);

                bsp_display_unlock();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Handy Keyboard Firmware v0.3.0");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "========================================");

    // Initialize NVS
    nvs_init();

    ESP_LOGI(TAG, "ESP32-P4 initialized");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    // Initialize BSP (Display, Touch, I2C, Audio)
    ESP_LOGI(TAG, "Initializing BSP...");
    g_display = bsp_display_start();
    if (g_display == NULL) {
        ESP_LOGE(TAG, "Failed to initialize display!");
        return;
    }
    ESP_LOGI(TAG, "Display initialized successfully");

    // Turn on backlight
    bsp_display_backlight_on();

    // Initialize RTC using BSP I2C handle
    rtc_init();

    // Initialize SquareLine Studio UI, navigation, and customization
    if (bsp_display_lock(1000)) {
        ui_init();
        ui_navigation_init();
        ui_customization_init();
        ESP_LOGI(TAG, "UI initialized with navigation and customization");
        bsp_display_unlock();
    } else {
        ESP_LOGE(TAG, "Failed to acquire display lock for UI creation");
    }

    // Start RTC update task (updates clock hands and header every second)
    if (g_rtc_initialized) {
        xTaskCreate(rtc_update_task, "rtc_update", 4096, NULL, 5, NULL);
    }

    // Initialize BLE HID keyboard
    ESP_LOGI(TAG, "Initializing BLE HID...");
    esp_err_t ble_ret = ble_hid_init();
    if (ble_ret != ESP_OK) {
        ESP_LOGW(TAG, "BLE HID init failed: %s", esp_err_to_name(ble_ret));
    } else {
        ESP_LOGI(TAG, "BLE HID initialized - device is now discoverable");
    }

    ESP_LOGI(TAG, "Initialization complete");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    // Disable most logging but keep BLE_HID for debugging
    // This prevents USB Serial/JTAG blocking while allowing BLE debugging
    ESP_LOGI(TAG, "Reducing runtime logs for standalone operation");
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("BLE_HID", ESP_LOG_INFO);

    // Main loop - LVGL timer handling is done by esp_lvgl_port
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
