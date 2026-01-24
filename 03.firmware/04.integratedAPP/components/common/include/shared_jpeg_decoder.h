/**
 * @file shared_jpeg_decoder.h
 * @brief Shared Hardware JPEG Decoder for ESP32-P4
 *
 * Provides a singleton JPEG decoder instance shared between ui_background
 * and mjpeg_player components. The ESP32-P4 hardware JPEG decoder requires
 * internal DMA memory, so sharing a single instance conserves memory.
 *
 * Usage:
 * 1. Call shared_jpeg_decoder_init() early in main() before display init
 * 2. Use shared_jpeg_decoder_lock() before decode operations
 * 3. Get handle with shared_jpeg_decoder_get_handle()
 * 4. Call shared_jpeg_decoder_unlock() after decode completes
 */

#pragma once

#include "esp_err.h"
#include "driver/jpeg_decode.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the shared hardware JPEG decoder
 *
 * This must be called EARLY in system initialization (before display init),
 * when internal DMA memory is still available. The hardware JPEG decoder
 * requires internal DMA-capable memory for its descriptor chains.
 *
 * Safe to call multiple times - subsequent calls are no-ops.
 *
 * @return
 *     - ESP_OK: Success (or already initialized)
 *     - ESP_ERR_NO_MEM: Not enough internal DMA memory
 */
esp_err_t shared_jpeg_decoder_init(void);

/**
 * @brief Get the shared JPEG decoder handle
 *
 * Returns the singleton decoder handle. Must call shared_jpeg_decoder_lock()
 * before using the handle, and shared_jpeg_decoder_unlock() after.
 *
 * @return JPEG decoder handle, or NULL if not initialized
 */
jpeg_decoder_handle_t shared_jpeg_decoder_get_handle(void);

/**
 * @brief Lock the shared decoder for exclusive use
 *
 * Must be called before any decode operation to ensure thread safety.
 * The decoder is shared between ui_background (static JPEG) and
 * mjpeg_player (video frames).
 *
 * @param timeout_ms Maximum time to wait for lock (milliseconds)
 * @return true if lock acquired, false on timeout
 */
bool shared_jpeg_decoder_lock(uint32_t timeout_ms);

/**
 * @brief Unlock the shared decoder
 *
 * Must be called after decode operation completes to allow other
 * components to use the decoder.
 */
void shared_jpeg_decoder_unlock(void);

/**
 * @brief Check if the shared decoder is initialized
 *
 * @return true if initialized and ready for use
 */
bool shared_jpeg_decoder_is_initialized(void);

#ifdef __cplusplus
}
#endif
