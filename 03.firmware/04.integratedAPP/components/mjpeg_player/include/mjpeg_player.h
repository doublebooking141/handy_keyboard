/**
 * @file mjpeg_player.h
 * @brief MJPEG Video Player for LVGL
 *
 * Plays MJPEG video files on LVGL image widgets using ESP32-P4 hardware JPEG decoder.
 * MJPEG format: Concatenated JPEG frames (FFD8...FFD9 markers)
 */

#pragma once

#include "esp_err.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** MJPEG player handle */
typedef struct mjpeg_player *mjpeg_player_handle_t;

/**
 * @brief Cancellation check callback type
 *
 * This callback is called periodically during frame indexing (mjpeg_player_create).
 * Return true to cancel the operation.
 *
 * @param user_data User data passed to the callback
 * @return true to cancel, false to continue
 */
typedef bool (*mjpeg_cancel_check_cb_t)(void *user_data);

/** MJPEG player configuration */
typedef struct {
    const char *file_path;      /**< Path to MJPEG file */
    lv_obj_t *target_image;     /**< LVGL image widget to display frames */
    uint16_t target_width;      /**< Target width for scaling (0 = original) */
    uint16_t target_height;     /**< Target height for scaling (0 = original) */
    uint8_t fps;                /**< Target FPS (1-60, default 30) */
    bool loop;                  /**< Loop playback (default true) */
    mjpeg_cancel_check_cb_t cancel_check; /**< Optional cancellation callback */
    void *cancel_user_data;     /**< User data for cancel callback */
} mjpeg_player_config_t;

/** MJPEG player state */
typedef enum {
    MJPEG_STATE_IDLE,           /**< Not playing */
    MJPEG_STATE_PLAYING,        /**< Currently playing */
    MJPEG_STATE_PAUSED,         /**< Paused */
    MJPEG_STATE_ERROR,          /**< Error occurred */
} mjpeg_player_state_t;

/**
 * @brief Create MJPEG player instance
 *
 * @param config Player configuration
 * @param[out] handle Pointer to store player handle
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - ESP_ERR_NO_MEM: Memory allocation failed
 *     - ESP_ERR_NOT_FOUND: File not found
 */
esp_err_t mjpeg_player_create(const mjpeg_player_config_t *config, mjpeg_player_handle_t *handle);

/**
 * @brief Destroy MJPEG player instance
 *
 * @param handle Player handle
 */
void mjpeg_player_destroy(mjpeg_player_handle_t handle);

/**
 * @brief Start playback
 *
 * @param handle Player handle
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_STATE: Already playing
 */
esp_err_t mjpeg_player_start(mjpeg_player_handle_t handle);

/**
 * @brief Stop playback
 *
 * @param handle Player handle
 * @return ESP_OK always
 */
esp_err_t mjpeg_player_stop(mjpeg_player_handle_t handle);

/**
 * @brief Pause playback
 *
 * @param handle Player handle
 * @return ESP_OK always
 */
esp_err_t mjpeg_player_pause(mjpeg_player_handle_t handle);

/**
 * @brief Resume playback
 *
 * @param handle Player handle
 * @return ESP_OK always
 */
esp_err_t mjpeg_player_resume(mjpeg_player_handle_t handle);

/**
 * @brief Get current playback state
 *
 * @param handle Player handle
 * @return Current state
 */
mjpeg_player_state_t mjpeg_player_get_state(mjpeg_player_handle_t handle);

/**
 * @brief Get playback progress
 *
 * @param handle Player handle
 * @param[out] current_frame Current frame number
 * @param[out] total_frames Total number of frames (0 if unknown)
 */
void mjpeg_player_get_progress(mjpeg_player_handle_t handle, uint32_t *current_frame, uint32_t *total_frames);

#ifdef __cplusplus
}
#endif
