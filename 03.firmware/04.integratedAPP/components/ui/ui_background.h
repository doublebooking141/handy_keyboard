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
 * @brief Initialize the shared hardware JPEG decoder
 *
 * This should be called EARLY in system initialization (before display init),
 * when internal DMA memory is still available. The hardware JPEG decoder
 * requires internal DMA-capable memory for its descriptor chains.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NO_MEM: Not enough internal DMA memory
 */
esp_err_t ui_bg_init_jpeg_decoder(void);

/**
 * @brief Apply background image to all touchpad panels
 *
 * Sets the same background image on JPKeyboard, AtoZ, and Cursor
 * screen touchpad panels.
 *
 * @param[in] path Full path to image file, or NULL to clear
 * @return ESP_OK on success
 */
esp_err_t ui_bg_apply_to_touchpad(const char *path);

/**
 * @brief Apply background image to analog clock
 *
 * @param[in] path Full path to image file, or NULL to use default
 * @return ESP_OK on success
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
