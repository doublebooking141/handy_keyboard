/**
 * @file shared_jpeg_decoder.c
 * @brief Shared Hardware JPEG Decoder Implementation
 *
 * Singleton JPEG decoder shared between ui_background and mjpeg_player.
 * Uses ESP32-P4 hardware JPEG decoder for fast decoding.
 */

#include "shared_jpeg_decoder.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "SHARED_JPEG";

// Singleton decoder instance
static jpeg_decoder_handle_t s_decoder = NULL;
static SemaphoreHandle_t s_mutex = NULL;

esp_err_t shared_jpeg_decoder_init(void)
{
    // Create mutex if not exists
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    // Already initialized
    if (s_decoder != NULL) {
        ESP_LOGD(TAG, "Shared JPEG decoder already initialized");
        return ESP_OK;
    }

    // Log available internal DMA memory before allocation
    size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Free internal DMA memory before JPEG init: %u bytes", free_dma);

    // Create hardware JPEG decoder
    jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .timeout_ms = 100,
    };
    esp_err_t ret = jpeg_new_decoder_engine(&decode_eng_cfg, &s_decoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create JPEG decoder: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Free internal DMA memory: %u bytes (may need more for rxlink)", free_dma);
        return ret;
    }

    ESP_LOGI(TAG, "Hardware JPEG decoder initialized successfully");
    return ESP_OK;
}

jpeg_decoder_handle_t shared_jpeg_decoder_get_handle(void)
{
    return s_decoder;
}

bool shared_jpeg_decoder_lock(uint32_t timeout_ms)
{
    if (s_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(s_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void shared_jpeg_decoder_unlock(void)
{
    if (s_mutex != NULL) {
        xSemaphoreGive(s_mutex);
    }
}

bool shared_jpeg_decoder_is_initialized(void)
{
    return s_decoder != NULL;
}
