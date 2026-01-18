/**
 * @file sdcard.c
 * @brief SD Card management implementation
 */

#include "sdcard.h"
#include "bsp/jc4880p443c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>  // strcasecmp

static const char *TAG = "SDCARD";

// NVS namespace and keys
#define NVS_NAMESPACE       "bg_settings"
#define NVS_KEY_TOUCHPAD    "bg_touchpad"
#define NVS_KEY_CLOCK       "bg_clock"

// State
static sdcard_state_t s_state = SDCARD_STATE_NOT_PRESENT;
static nvs_handle_t s_nvs_handle = 0;
static bool s_nvs_opened = false;

/**
 * @brief Check if filename has supported image extension
 */
static bool is_image_file(const char *filename)
{
    if (!filename || strlen(filename) < 5) {
        return false;
    }

    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return false;
    }

    // Supported formats (LVGL can decode these + MJPEG video)
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".bmp") == 0 ||
            strcasecmp(ext, ".mjpg") == 0 ||
            strcasecmp(ext, ".mjpeg") == 0);
}

/**
 * @brief Open NVS namespace for settings
 */
static esp_err_t open_nvs(void)
{
    if (s_nvs_opened) {
        return ESP_OK;
    }

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret == ESP_OK) {
        s_nvs_opened = true;
    } else {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t sdcard_init(void)
{
    ESP_LOGI(TAG, "Initializing SD card management...");

    // Try to mount SD card using BSP
    esp_err_t ret = bsp_sdcard_mount();
    if (ret == ESP_OK) {
        s_state = SDCARD_STATE_MOUNTED;
        ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);

        // Create background directory if it doesn't exist
        struct stat st;
        if (stat(SDCARD_BG_DIR, &st) != 0) {
            if (mkdir(SDCARD_BG_DIR, 0755) == 0) {
                ESP_LOGI(TAG, "Created background directory: %s", SDCARD_BG_DIR);
            } else {
                ESP_LOGW(TAG, "Failed to create background directory");
            }
        }
    } else {
        s_state = SDCARD_STATE_NOT_PRESENT;
        ESP_LOGW(TAG, "SD card not available: %s", esp_err_to_name(ret));
    }

    // Open NVS for settings (independent of SD card)
    open_nvs();

    return ESP_OK;  // Always return OK - SD card is optional
}

sdcard_state_t sdcard_get_state(void)
{
    return s_state;
}

bool sdcard_is_available(void)
{
    return s_state == SDCARD_STATE_MOUNTED;
}

esp_err_t sdcard_list_backgrounds(sdcard_file_t *files, size_t max_files, size_t *count)
{
    if (!files || !count) {
        return ESP_ERR_INVALID_ARG;
    }

    *count = 0;

    if (s_state != SDCARD_STATE_MOUNTED) {
        return ESP_ERR_INVALID_STATE;
    }

    DIR *dir = opendir(SDCARD_BG_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Cannot open background directory: %s", SDCARD_BG_DIR);
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < max_files) {
        // Skip directories and hidden files
        if (entry->d_type == DT_DIR || entry->d_name[0] == '.') {
            continue;
        }

        // Check if it's an image file
        if (!is_image_file(entry->d_name)) {
            continue;
        }

        // Store file info
        strncpy(files[*count].filename, entry->d_name, sizeof(files[*count].filename) - 1);
        files[*count].filename[sizeof(files[*count].filename) - 1] = '\0';

        snprintf(files[*count].full_path, sizeof(files[*count].full_path),
                 "%s/%s", SDCARD_BG_DIR, entry->d_name);

        ESP_LOGD(TAG, "Found: %s", files[*count].filename);
        (*count)++;
    }

    closedir(dir);

    ESP_LOGI(TAG, "Found %zu background images", *count);
    return ESP_OK;
}

esp_err_t sdcard_refresh(void)
{
    ESP_LOGI(TAG, "Refreshing SD card...");

    // Unmount if mounted
    if (s_state == SDCARD_STATE_MOUNTED) {
        bsp_sdcard_unmount();
        s_state = SDCARD_STATE_NOT_PRESENT;
    }

    // Try to remount
    esp_err_t ret = bsp_sdcard_mount();
    if (ret == ESP_OK) {
        s_state = SDCARD_STATE_MOUNTED;
        ESP_LOGI(TAG, "SD card refreshed successfully");
    } else {
        s_state = SDCARD_STATE_NOT_PRESENT;
        ESP_LOGW(TAG, "SD card not available after refresh");
    }

    return ret;
}

esp_err_t sdcard_format(void)
{
    ESP_LOGW(TAG, "Formatting SD card (all data will be lost!)");

    // Check if SD card is currently mounted
    if (s_state != SDCARD_STATE_MOUNTED) {
        ESP_LOGW(TAG, "SD card not mounted, trying to mount first...");
        esp_err_t mount_ret = bsp_sdcard_mount();
        if (mount_ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot format: SD card mount failed (%s)", esp_err_to_name(mount_ret));
            ESP_LOGE(TAG, "Please check if SD card is inserted properly");
            s_state = SDCARD_STATE_NOT_PRESENT;
            return mount_ret;
        }
        s_state = SDCARD_STATE_MOUNTED;
    }

    esp_err_t ret = bsp_sdcard_format();
    if (ret == ESP_OK) {
        s_state = SDCARD_STATE_MOUNTED;
        ESP_LOGI(TAG, "SD card formatted successfully");

        // Recreate background directory
        mkdir(SDCARD_BG_DIR, 0755);
    } else {
        s_state = SDCARD_STATE_ERROR;
        ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t sdcard_get_touchpad_bg(char *path, size_t len)
{
    if (!path || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_nvs_opened && open_nvs() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t required_size = len;
    esp_err_t ret = nvs_get_str(s_nvs_handle, NVS_KEY_TOUCHPAD, path, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        path[0] = '\0';
    }
    return ret;
}

esp_err_t sdcard_set_touchpad_bg(const char *path)
{
    if (!s_nvs_opened && open_nvs() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;
    if (path && path[0] != '\0') {
        ret = nvs_set_str(s_nvs_handle, NVS_KEY_TOUCHPAD, path);
    } else {
        ret = nvs_erase_key(s_nvs_handle, NVS_KEY_TOUCHPAD);
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ret = ESP_OK;  // Key didn't exist, that's fine
        }
    }

    if (ret == ESP_OK) {
        ret = nvs_commit(s_nvs_handle);
    }

    ESP_LOGI(TAG, "Touchpad background set: %s", path ? path : "(none)");
    return ret;
}

esp_err_t sdcard_get_clock_bg(char *path, size_t len)
{
    if (!path || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_nvs_opened && open_nvs() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t required_size = len;
    esp_err_t ret = nvs_get_str(s_nvs_handle, NVS_KEY_CLOCK, path, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        path[0] = '\0';
    }
    return ret;
}

esp_err_t sdcard_set_clock_bg(const char *path)
{
    if (!s_nvs_opened && open_nvs() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;
    if (path && path[0] != '\0') {
        ret = nvs_set_str(s_nvs_handle, NVS_KEY_CLOCK, path);
    } else {
        ret = nvs_erase_key(s_nvs_handle, NVS_KEY_CLOCK);
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ret = ESP_OK;
        }
    }

    if (ret == ESP_OK) {
        ret = nvs_commit(s_nvs_handle);
    }

    ESP_LOGI(TAG, "Clock background set: %s", path ? path : "(none)");
    return ret;
}

esp_err_t sdcard_get_capacity(uint32_t *total_mb, uint32_t *used_mb)
{
    if (!total_mb || !used_mb) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state != SDCARD_STATE_MOUNTED || !bsp_sdcard) {
        return ESP_ERR_INVALID_STATE;
    }

    // Get card info from BSP global
    uint64_t total_bytes = (uint64_t)bsp_sdcard->csd.capacity * bsp_sdcard->csd.sector_size;
    *total_mb = (uint32_t)(total_bytes / (1024 * 1024));

    // Get used space via FATFS (simplified - just report total for now)
    // A full implementation would use f_getfree()
    *used_mb = 0;  // TODO: implement actual used space calculation

    ESP_LOGD(TAG, "SD card capacity: %lu MB", (unsigned long)*total_mb);
    return ESP_OK;
}
