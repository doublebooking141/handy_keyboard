#pragma once
#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Hardware Abstraction Layer - Audio
 */

esp_err_t hal_audio_init(void);
esp_err_t hal_audio_write_i2s(const void* data, size_t size, size_t* bytes_written, uint32_t timeout_ms);
esp_err_t hal_audio_set_pa_enable(bool enable);
esp_err_t hal_audio_codec_set_vol(uint8_t vol);
