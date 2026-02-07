/**
 * @file ui_background.h
 * @brief Background image management for UI screens
 *
 * Applies background images from SD card to touchpad panels
 * and analog clock container. Uses ESP32-P4 hardware JPEG decoder
 * for fast JPEG loading.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Background loading state
 */
typedef enum {
    UI_BG_STATE_NONE,      /**< No background set */
    UI_BG_STATE_PENDING,   /**< Path set, waiting to load */
    UI_BG_STATE_LOADING,   /**< Loading in progress */
    UI_BG_STATE_READY,     /**< Loaded and ready */
} ui_bg_state_t;

/**
 * @brief Set touchpad background path (deferred loading)
 *
 * Only saves the path. Actual loading happens when screen is displayed.
 *
 * @param[in] path Full path to image file, or NULL to clear
 */
void ui_bg_set_touchpad_path(const char *path);

/**
 * @brief Set clock background path (deferred loading)
 *
 * Only saves the path. Actual loading happens when screen is displayed.
 *
 * @param[in] path Full path to image file, or NULL to clear
 */
void ui_bg_set_clock_path(const char *path);

/**
 * @brief Check and start loading backgrounds if needed
 *
 * Call this when navigating to a screen that needs background.
 * Shows spinner during loading.
 */
void ui_bg_check_and_load(void);

/**
 * @brief Get touchpad background loading state
 */
ui_bg_state_t ui_bg_get_touchpad_state(void);

/**
 * @brief Get clock background loading state
 */
ui_bg_state_t ui_bg_get_clock_state(void);

/**
 * @brief Apply background image to all touchpad panels (immediate)
 *
 * Sets the same background image on JPKeyboard, AtoZ, and Cursor
 * screen touchpad panels.
 *
 * @param[in] path Full path to image file, or NULL to clear
 * @return ESP_OK on success
 * @note Prefer ui_bg_set_touchpad_path() for deferred loading
 */
esp_err_t ui_bg_apply_to_touchpad(const char *path);

/**
 * @brief Apply background image to analog clock (immediate)
 *
 * @param[in] path Full path to image file, or NULL to use default
 * @return ESP_OK on success
 * @note Prefer ui_bg_set_clock_path() for deferred loading
 */
esp_err_t ui_bg_apply_to_clock(const char *path);

/**
 * @brief Clear touchpad background (restore default)
 *
 * @return ESP_OK on success
 */
esp_err_t ui_bg_clear_touchpad(void);

/**
 * @brief Clear clock background (restore default)
 *
 * @return ESP_OK on success
 */
esp_err_t ui_bg_clear_clock(void);

/**
 * @brief Load and apply saved background settings from NVS
 *
 * Call this after UI and SD card are initialized to restore
 * user's background preferences.
 */
void ui_bg_load_saved_settings(void);

/**
 * @brief Refresh backgrounds when screens are recreated
 *
 * Called by ui_navigation_update() to reapply backgrounds
 * after screen transitions.
 */
void ui_bg_refresh_if_needed(void);

#ifdef __cplusplus
}
#endif
