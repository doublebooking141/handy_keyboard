/**
 * @file audio_player.h
 * @brief Audio Player API - WAV file playback via ES8311 codec
 *
 * Provides background audio playback from SD card WAV files.
 * Uses FreeRTOS task for non-blocking playback.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio player state
 */
typedef enum {
    AUDIO_PLAYER_STATE_IDLE,      /**< Not playing */
    AUDIO_PLAYER_STATE_PLAYING,   /**< Currently playing */
    AUDIO_PLAYER_STATE_PAUSED,    /**< Playback paused */
    AUDIO_PLAYER_STATE_ERROR      /**< Error occurred */
} audio_player_state_t;

/**
 * @brief Playback completion callback
 *
 * @param user_data User-provided context
 * @param completed true if playback finished normally, false if stopped
 */
typedef void (*audio_player_callback_t)(void *user_data, bool completed);

/**
 * @brief Initialize the audio player
 *
 * Initializes the ES8311 codec via BSP and prepares for playback.
 * Must be called before other audio_player functions.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_player_init(void);

/**
 * @brief Deinitialize the audio player
 *
 * Stops any active playback and releases resources.
 */
void audio_player_deinit(void);

/**
 * @brief Play a WAV file from SD card
 *
 * Starts background playback of the specified WAV file.
 * Supports PCM WAV files (8/16-bit, mono/stereo, 8000-48000Hz).
 *
 * @param file_path Full path to WAV file (e.g., "/sdcard/sounds/alarm.wav")
 * @param volume Volume level (0-100)
 * @param loop If true, loops playback until stopped
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_player_play_file(const char *file_path, uint8_t volume, bool loop);

/**
 * @brief Stop current playback
 *
 * @return ESP_OK on success
 */
esp_err_t audio_player_stop(void);

/**
 * @brief Pause current playback
 *
 * @return ESP_OK on success
 */
esp_err_t audio_player_pause(void);

/**
 * @brief Resume paused playback
 *
 * @return ESP_OK on success
 */
esp_err_t audio_player_resume(void);

/**
 * @brief Set playback volume
 *
 * @param volume Volume level (0-100)
 * @return ESP_OK on success
 */
esp_err_t audio_player_set_volume(uint8_t volume);

/**
 * @brief Get current volume level
 *
 * @return Current volume (0-100)
 */
uint8_t audio_player_get_volume(void);

/**
 * @brief Check if audio is currently playing
 *
 * @return true if playing, false otherwise
 */
bool audio_player_is_playing(void);

/**
 * @brief Get current player state
 *
 * @return Current state
 */
audio_player_state_t audio_player_get_state(void);

/**
 * @brief Register playback completion callback
 *
 * Callback is invoked when playback completes or is stopped.
 *
 * @param callback Callback function (NULL to unregister)
 * @param user_data User context passed to callback
 * @return ESP_OK on success
 */
esp_err_t audio_player_register_callback(audio_player_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif
