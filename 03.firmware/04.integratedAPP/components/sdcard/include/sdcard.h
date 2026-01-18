/**
 * @file sdcard.h
 * @brief SD Card management for background images
 *
 * Provides file listing, state management, and NVS persistence
 * for background image settings.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SD card states
 */
typedef enum {
    SDCARD_STATE_NOT_PRESENT,   /**< SD card not inserted or mount failed */
    SDCARD_STATE_MOUNTED,       /**< SD card mounted and ready */
    SDCARD_STATE_ERROR,         /**< SD card error occurred */
} sdcard_state_t;

/**
 * @brief File information structure
 */
typedef struct {
    char filename[64];          /**< File name only */
    char full_path[128];        /**< Full path for LVGL */
} sdcard_file_t;

/** Maximum number of background files to list */
#define SDCARD_MAX_FILES        32

/** Background images directory */
#define SDCARD_BG_DIR           "/sdcard/background"

/** Special value for "no background" */
#define SDCARD_BG_NONE          ""

/**
 * @brief Initialize SD card management
 *
 * Mounts SD card using BSP and initializes NVS namespace.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sdcard_init(void);

/**
 * @brief Get current SD card state
 *
 * @return Current state (mounted, not present, or error)
 */
sdcard_state_t sdcard_get_state(void);

/**
 * @brief Check if SD card is available for use
 *
 * @return true if mounted and ready
 */
bool sdcard_is_available(void);

/**
 * @brief List background image files from SD card
 *
 * Lists files in /sdcard/background/ directory.
 * Supports common image formats (jpg, jpeg, png, bmp).
 *
 * @param[out] files Array to store file information
 * @param[in]  max_files Maximum files to retrieve
 * @param[out] count Actual number of files found
 * @return ESP_OK on success
 */
esp_err_t sdcard_list_backgrounds(sdcard_file_t *files, size_t max_files, size_t *count);

/**
 * @brief Refresh SD card (remount and rescan)
 *
 * @return ESP_OK on success
 */
esp_err_t sdcard_refresh(void);

/**
 * @brief Format SD card as FAT32
 *
 * WARNING: All data will be erased!
 *
 * @return ESP_OK on success
 */
esp_err_t sdcard_format(void);

/**
 * @brief Get saved touchpad background path
 *
 * @param[out] path Buffer to store path
 * @param[in]  len  Buffer length
 * @return ESP_OK if path exists, ESP_ERR_NOT_FOUND if not set
 */
esp_err_t sdcard_get_touchpad_bg(char *path, size_t len);

/**
 * @brief Set touchpad background path (persisted to NVS)
 *
 * @param[in] path Full path to image, or SDCARD_BG_NONE to clear
 * @return ESP_OK on success
 */
esp_err_t sdcard_set_touchpad_bg(const char *path);

/**
 * @brief Get saved clock background path
 *
 * @param[out] path Buffer to store path
 * @param[in]  len  Buffer length
 * @return ESP_OK if path exists, ESP_ERR_NOT_FOUND if not set
 */
esp_err_t sdcard_get_clock_bg(char *path, size_t len);

/**
 * @brief Set clock background path (persisted to NVS)
 *
 * @param[in] path Full path to image, or SDCARD_BG_NONE to clear
 * @return ESP_OK on success
 */
esp_err_t sdcard_set_clock_bg(const char *path);

/**
 * @brief Get SD card capacity information
 *
 * @param[out] total_mb Total capacity in MB
 * @param[out] used_mb  Used space in MB
 * @return ESP_OK on success
 */
esp_err_t sdcard_get_capacity(uint32_t *total_mb, uint32_t *used_mb);

#ifdef __cplusplus
}
#endif
