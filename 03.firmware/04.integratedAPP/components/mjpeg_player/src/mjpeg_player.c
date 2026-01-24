/**
 * @file mjpeg_player.c
 * @brief MJPEG Video Player Implementation using ESP32-P4 Hardware JPEG Decoder
 *
 * Uses a dedicated FreeRTOS task for frame decoding to avoid blocking.
 */

#include "mjpeg_player.h"
#include "shared_jpeg_decoder.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/jpeg_decode.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "bsp/jc4880p443c.h"  // For bsp_display_lock/unlock
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "MJPEG_PLAYER";

/** JPEG markers */
#define JPEG_SOI_MARKER 0xFFD8  /**< Start Of Image */
#define JPEG_EOI_MARKER 0xFFD9  /**< End Of Image */

/** Buffer sizes */
#define MJPEG_READ_BUFFER_SIZE (64 * 1024)   /**< 64KB read buffer for faster scanning */
#define MJPEG_MAX_FRAME_SIZE   (256 * 1024)  /**< 256KB max frame size */
#define MJPEG_MAX_FRAMES       5000          /**< Maximum frames to index (~3.5 min at 24fps) */

/** Task configuration */
#define MJPEG_TASK_STACK_SIZE  8192
#define MJPEG_TASK_PRIORITY    7  // Higher priority for smoother playback

/** Frame index entry for seeking */
typedef struct {
    uint32_t offset;            /**< File offset */
    uint32_t size;              /**< Frame size in bytes */
} mjpeg_frame_index_t;

/** MJPEG player internal structure */
struct mjpeg_player {
    FILE *file;                 /**< File handle */
    char *file_path;            /**< File path copy */
    lv_obj_t *target_image;     /**< Target LVGL image widget */

    uint16_t target_width;      /**< Target width */
    uint16_t target_height;     /**< Target height */
    uint8_t fps;                /**< Target FPS */
    bool loop;                  /**< Loop playback */

    mjpeg_player_state_t state; /**< Current state */

    // Cancellation callback
    mjpeg_cancel_check_cb_t cancel_check; /**< Cancellation check callback */
    void *cancel_user_data;     /**< User data for cancel callback */

    // Playback task
    TaskHandle_t playback_task; /**< Playback task handle */
    volatile bool task_running; /**< Task running flag */

    // Input buffer (JPEG data)
    uint8_t *jpeg_buffer;       /**< JPEG input buffer (DMA capable) */
    size_t jpeg_buffer_size;    /**< JPEG buffer allocated size */

    // Double buffering for RGB output
    uint8_t *rgb_buffers[2];    /**< Two RGB output buffers for double buffering */
    size_t rgb_buffer_size;     /**< Size of each RGB buffer */
    lv_image_dsc_t image_dscs[2]; /**< LVGL image descriptors for each buffer */
    volatile uint8_t back_idx;  /**< Index of back buffer (0 or 1) */

    mjpeg_frame_index_t *frame_index; /**< Frame index array */
    uint32_t frame_count;       /**< Total number of frames */
    uint32_t current_frame;     /**< Current frame index */

    SemaphoreHandle_t mutex;    /**< Thread safety mutex */
};

// ============================================================================
// Internal Helpers
// ============================================================================

/**
 * @brief Fast frame indexing using buffered reads
 */
static esp_err_t index_frames_fast(struct mjpeg_player *player)
{
    fseek(player->file, 0, SEEK_END);
    long file_size = ftell(player->file);
    fseek(player->file, 0, SEEK_SET);

    ESP_LOGD(TAG, "Indexing MJPEG file (%ld bytes)...", file_size);

    // Allocate temporary read buffer from PSRAM (prefer) or internal RAM
    uint8_t *buffer = heap_caps_malloc(MJPEG_READ_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer == NULL) {
        // Fallback to internal RAM if PSRAM not available
        buffer = heap_caps_malloc(MJPEG_READ_BUFFER_SIZE, MALLOC_CAP_DEFAULT);
    }
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate read buffer");
        return ESP_ERR_NO_MEM;
    }

    // Temporary frame index from PSRAM (will realloc to exact size later)
    mjpeg_frame_index_t *temp_index = heap_caps_malloc(MJPEG_MAX_FRAMES * sizeof(mjpeg_frame_index_t),
                                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (temp_index == NULL) {
        // Fallback to internal RAM
        temp_index = malloc(MJPEG_MAX_FRAMES * sizeof(mjpeg_frame_index_t));
    }
    if (temp_index == NULL) {
        free(buffer);
        ESP_LOGE(TAG, "Failed to allocate temp index");
        return ESP_ERR_NO_MEM;
    }

    uint32_t frame_count = 0;
    uint32_t file_offset = 0;
    bool in_frame = false;
    uint32_t frame_start = 0;
    uint8_t prev_byte = 0;

    while (file_offset < (uint32_t)file_size && frame_count < MJPEG_MAX_FRAMES) {
        size_t to_read = MJPEG_READ_BUFFER_SIZE;
        if (file_offset + to_read > (uint32_t)file_size) {
            to_read = file_size - file_offset;
        }

        size_t bytes_read = fread(buffer, 1, to_read, player->file);
        if (bytes_read == 0) break;

        for (size_t i = 0; i < bytes_read; i++) {
            uint8_t curr_byte = buffer[i];
            uint16_t marker = (prev_byte << 8) | curr_byte;

            if (!in_frame) {
                if (marker == JPEG_SOI_MARKER) {
                    in_frame = true;
                    frame_start = file_offset + i - 1;
                }
            } else {
                if (marker == JPEG_EOI_MARKER) {
                    uint32_t frame_end = file_offset + i + 1;
                    temp_index[frame_count].offset = frame_start;
                    temp_index[frame_count].size = frame_end - frame_start;
                    frame_count++;
                    in_frame = false;
                }
            }

            prev_byte = curr_byte;
        }

        file_offset += bytes_read;

        // Yield to avoid watchdog during large file scan
        vTaskDelay(1);

        // Check for cancellation
        if (player->cancel_check != NULL && player->cancel_check(player->cancel_user_data)) {
            ESP_LOGD(TAG, "Frame indexing cancelled at %lu/%ld bytes", (unsigned long)file_offset, file_size);
            free(buffer);
            free(temp_index);
            return ESP_ERR_INVALID_STATE;  // Indicate cancellation
        }
    }

    free(buffer);

    if (frame_count == 0) {
        free(temp_index);
        ESP_LOGE(TAG, "No JPEG frames found in file");
        return ESP_ERR_INVALID_SIZE;
    }

    // Shrink index to actual size (keep in PSRAM if that's where it was allocated)
    mjpeg_frame_index_t *shrunk_index = heap_caps_realloc(temp_index,
                                                          frame_count * sizeof(mjpeg_frame_index_t),
                                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (shrunk_index == NULL) {
        // Try without PSRAM flag
        shrunk_index = realloc(temp_index, frame_count * sizeof(mjpeg_frame_index_t));
    }
    player->frame_index = (shrunk_index != NULL) ? shrunk_index : temp_index;
    player->frame_count = frame_count;

    ESP_LOGI(TAG, "Found %lu frames in MJPEG file", (unsigned long)frame_count);

    return ESP_OK;
}

/**
 * @brief Decode and display a frame using hardware JPEG decoder with double buffering
 *
 * Double buffering flow:
 * 1. Decode JPEG to back buffer (no display lock needed)
 * 2. Acquire display lock briefly
 * 3. Swap front/back buffers (pointer switch only)
 * 4. Release display lock
 *
 * This minimizes display lock hold time to reduce frame skipping.
 */
static esp_err_t decode_and_display_frame(struct mjpeg_player *player, uint32_t frame_idx)
{
    if (frame_idx >= player->frame_count) {
        return ESP_ERR_INVALID_ARG;
    }

    mjpeg_frame_index_t *frame = &player->frame_index[frame_idx];

    // Check frame size
    if (frame->size > player->jpeg_buffer_size) {
        ESP_LOGE(TAG, "Frame size %lu exceeds buffer %lu",
                 (unsigned long)frame->size, (unsigned long)player->jpeg_buffer_size);
        return ESP_ERR_NO_MEM;
    }

    // Get current back buffer index (decode target)
    uint8_t back = player->back_idx;
    uint8_t *decode_buffer = player->rgb_buffers[back];
    lv_image_dsc_t *decode_dsc = &player->image_dscs[back];

    // Read JPEG frame data (no lock needed)
    fseek(player->file, frame->offset, SEEK_SET);
    size_t bytes_read = fread(player->jpeg_buffer, 1, frame->size, player->file);
    if (bytes_read != frame->size) {
        ESP_LOGE(TAG, "Failed to read frame %lu", (unsigned long)frame_idx);
        return ESP_FAIL;
    }

    // Get JPEG info to get actual dimensions
    jpeg_decode_picture_info_t info;
    esp_err_t ret = jpeg_decoder_get_info(player->jpeg_buffer, frame->size, &info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get JPEG info: %s", esp_err_to_name(ret));
        return ret;
    }

    if (frame_idx == 0) {
        ESP_LOGD(TAG, "Frame size: %lux%lu", (unsigned long)info.width, (unsigned long)info.height);
    }

    // Check if output buffer is large enough for RGB565
    // Hardware JPEG decoder aligns output to 16-byte boundaries
    uint32_t aligned_width = (info.width + 15) & ~15;
    uint32_t aligned_height = (info.height + 15) & ~15;
    size_t required_size = aligned_width * aligned_height * 2;  // RGB565 = 2 bytes per pixel
    if (required_size > player->rgb_buffer_size) {
        ESP_LOGE(TAG, "RGB buffer too small: need %lu, have %lu",
                 (unsigned long)required_size, (unsigned long)player->rgb_buffer_size);
        return ESP_ERR_NO_MEM;
    }

    // Decode JPEG to RGB565 using hardware decoder (matches display format)
    // Decoding to back buffer - no display lock needed here
    jpeg_decode_cfg_t decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,  // Display expects BGR order
    };

    // Lock shared decoder for exclusive access during decode
    if (!shared_jpeg_decoder_lock(100)) {
        ESP_LOGW(TAG, "Decoder busy, skipping frame %lu", (unsigned long)frame_idx);
        return ESP_ERR_TIMEOUT;
    }

    uint32_t out_size = 0;
    ret = jpeg_decoder_process(
        shared_jpeg_decoder_get_handle(),
        &decode_cfg,
        player->jpeg_buffer,
        frame->size,
        decode_buffer,  // Decode to back buffer
        player->rgb_buffer_size,
        &out_size
    );

    shared_jpeg_decoder_unlock();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware JPEG decode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update back buffer's LVGL image descriptor (no lock needed)
    decode_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    decode_dsc->header.w = info.width;
    decode_dsc->header.h = info.height;
    decode_dsc->header.stride = info.width * 2;  // RGB565 = 2 bytes per pixel
    decode_dsc->data_size = out_size;
    decode_dsc->data = decode_buffer;

    // Brief display lock for buffer swap only
    if (bsp_display_lock(pdMS_TO_TICKS(50))) {
        lv_image_set_src(player->target_image, decode_dsc);
        lv_obj_invalidate(player->target_image);  // Force redraw
        player->back_idx = 1 - back;  // Swap buffers (0->1 or 1->0)
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "Display lock timeout, skipping frame %lu", (unsigned long)frame_idx);
    }

    player->current_frame = frame_idx;

    return ESP_OK;
}

/**
 * @brief Playback task - runs frame decoding in a dedicated task
 */
static void mjpeg_playback_task(void *arg)
{
    struct mjpeg_player *player = (struct mjpeg_player *)arg;
    TickType_t frame_delay = pdMS_TO_TICKS(1000 / player->fps);
    TickType_t last_wake_time = xTaskGetTickCount();

    ESP_LOGD(TAG, "Playback task started (delay=%lu ms)", (unsigned long)(1000 / player->fps));

    while (player->task_running) {
        if (player->state == MJPEG_STATE_PLAYING) {
            if (xSemaphoreTake(player->mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                uint32_t next_frame = player->current_frame + 1;

                if (next_frame >= player->frame_count) {
                    if (player->loop) {
                        next_frame = 0;
                    } else {
                        player->state = MJPEG_STATE_IDLE;
                        xSemaphoreGive(player->mutex);
                        continue;
                    }
                }

                decode_and_display_frame(player, next_frame);
                xSemaphoreGive(player->mutex);
            }

            // Wait for next frame time
            vTaskDelayUntil(&last_wake_time, frame_delay);
        } else if (player->state == MJPEG_STATE_PAUSED) {
            // Just wait when paused
            vTaskDelay(pdMS_TO_TICKS(100));
            last_wake_time = xTaskGetTickCount();
        } else {
            // Idle state - wait and check periodically
            vTaskDelay(pdMS_TO_TICKS(100));
            last_wake_time = xTaskGetTickCount();
        }
    }

    ESP_LOGD(TAG, "Playback task stopped");
    vTaskDelete(NULL);
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t mjpeg_player_create(const mjpeg_player_config_t *config, mjpeg_player_handle_t *handle)
{
    if (config == NULL || handle == NULL || config->file_path == NULL || config->target_image == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *handle = NULL;
    esp_err_t ret = ESP_ERR_NO_MEM;

    // Allocate player structure (calloc zeros all pointers for safe cleanup)
    struct mjpeg_player *player = calloc(1, sizeof(struct mjpeg_player));
    if (player == NULL) {
        ESP_LOGE(TAG, "Failed to allocate player structure");
        return ESP_ERR_NO_MEM;
    }

    // Copy file path
    player->file_path = strdup(config->file_path);
    if (player->file_path == NULL) {
        goto cleanup;
    }

    // Open file
    player->file = fopen(config->file_path, "rb");
    if (player->file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", config->file_path);
        ret = ESP_ERR_NOT_FOUND;
        goto cleanup;
    }

    // Create mutex
    player->mutex = xSemaphoreCreateMutex();
    if (player->mutex == NULL) {
        goto cleanup;
    }

    // Initialize settings
    player->target_image = config->target_image;
    player->target_width = config->target_width > 0 ? config->target_width : 480;
    player->target_height = config->target_height > 0 ? config->target_height : 800;
    player->fps = (config->fps > 0 && config->fps <= 60) ? config->fps : 24;
    player->loop = config->loop;
    player->state = MJPEG_STATE_IDLE;
    player->cancel_check = config->cancel_check;
    player->cancel_user_data = config->cancel_user_data;

    // Verify shared JPEG decoder is initialized
    if (!shared_jpeg_decoder_is_initialized()) {
        ESP_LOGE(TAG, "Shared JPEG decoder not initialized - call shared_jpeg_decoder_init() first");
        ret = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    // Allocate DMA-capable JPEG input buffer
    jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
    };
    player->jpeg_buffer = (uint8_t *)jpeg_alloc_decoder_mem(MJPEG_MAX_FRAME_SIZE, &tx_mem_cfg, &player->jpeg_buffer_size);
    if (player->jpeg_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate JPEG input buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    // Allocate DMA-capable RGB output buffers (double buffering)
    // Use larger dimension to handle both landscape and portrait videos
    // Also align to 16-byte boundaries as required by hardware JPEG decoder
    size_t max_dim = (player->target_width > player->target_height) ? player->target_width : player->target_height;
    size_t aligned_dim = (max_dim + 15) & ~15;  // Align to 16 bytes
    size_t rgb_size = aligned_dim * aligned_dim * 2;  // RGB565 = 2 bytes per pixel
    jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };

    // Allocate two RGB buffers for double buffering
    for (int i = 0; i < 2; i++) {
        size_t actual_size = 0;
        player->rgb_buffers[i] = (uint8_t *)jpeg_alloc_decoder_mem(rgb_size, &rx_mem_cfg, &actual_size);
        if (player->rgb_buffers[i] == NULL) {
            ESP_LOGE(TAG, "Failed to allocate RGB output buffer %d", i);
            ret = ESP_ERR_NO_MEM;
            goto cleanup;
        }
        if (i == 0) {
            player->rgb_buffer_size = actual_size;
        }
    }
    player->back_idx = 0;  // Start with buffer 0 as back buffer

    ESP_LOGD(TAG, "Allocated buffers: JPEG=%lu, RGB=%lux2 (double buffering)",
             (unsigned long)player->jpeg_buffer_size, (unsigned long)player->rgb_buffer_size);

    // Index frames (using fast buffered method)
    ret = index_frames_fast(player);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    // Create playback task
    player->task_running = true;
    BaseType_t task_ret = xTaskCreate(
        mjpeg_playback_task,
        "mjpeg_play",
        MJPEG_TASK_STACK_SIZE,
        player,
        MJPEG_TASK_PRIORITY,
        &player->playback_task
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create playback task");
        player->task_running = false;
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    ESP_LOGI(TAG, "MJPEG player created: %s (%lu frames, %d FPS, HW decoder)",
             config->file_path, (unsigned long)player->frame_count, player->fps);

    *handle = player;
    return ESP_OK;

cleanup:
    // Free resources in reverse allocation order (NULL-safe)
    free(player->frame_index);
    for (int i = 0; i < 2; i++) {
        free(player->rgb_buffers[i]);
    }
    free(player->jpeg_buffer);
    if (player->mutex != NULL) {
        vSemaphoreDelete(player->mutex);
    }
    if (player->file != NULL) {
        fclose(player->file);
    }
    free(player->file_path);
    free(player);
    return ret;
}

void mjpeg_player_destroy(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    struct mjpeg_player *player = handle;

    // Stop task
    player->task_running = false;
    player->state = MJPEG_STATE_IDLE;
    vTaskDelay(pdMS_TO_TICKS(200));  // Wait for task to finish

    // Free resources
    if (player->mutex != NULL) {
        vSemaphoreDelete(player->mutex);
    }

    if (player->frame_index != NULL) {
        free(player->frame_index);
    }

    // Free both RGB buffers (double buffering)
    for (int i = 0; i < 2; i++) {
        if (player->rgb_buffers[i] != NULL) {
            free(player->rgb_buffers[i]);
        }
    }

    if (player->jpeg_buffer != NULL) {
        free(player->jpeg_buffer);
    }

    if (player->file != NULL) {
        fclose(player->file);
    }

    if (player->file_path != NULL) {
        free(player->file_path);
    }

    free(player);

    ESP_LOGD(TAG, "MJPEG player destroyed");
}

esp_err_t mjpeg_player_start(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct mjpeg_player *player = handle;

    if (player->state == MJPEG_STATE_PLAYING) {
        return ESP_OK;  // Already playing
    }

    if (xSemaphoreTake(player->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Display first frame
    player->current_frame = 0;
    esp_err_t ret = decode_and_display_frame(player, 0);
    if (ret != ESP_OK) {
        xSemaphoreGive(player->mutex);
        return ret;
    }

    player->state = MJPEG_STATE_PLAYING;
    xSemaphoreGive(player->mutex);

    ESP_LOGD(TAG, "MJPEG playback started");
    return ESP_OK;
}

esp_err_t mjpeg_player_stop(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct mjpeg_player *player = handle;

    player->state = MJPEG_STATE_IDLE;
    player->current_frame = 0;

    ESP_LOGD(TAG, "MJPEG playback stopped");
    return ESP_OK;
}

esp_err_t mjpeg_player_pause(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct mjpeg_player *player = handle;

    if (player->state == MJPEG_STATE_PLAYING) {
        player->state = MJPEG_STATE_PAUSED;
        ESP_LOGD(TAG, "MJPEG playback paused at frame %lu", (unsigned long)player->current_frame);
    }

    return ESP_OK;
}

esp_err_t mjpeg_player_resume(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct mjpeg_player *player = handle;

    if (player->state == MJPEG_STATE_PAUSED) {
        player->state = MJPEG_STATE_PLAYING;
        ESP_LOGD(TAG, "MJPEG playback resumed");
    }

    return ESP_OK;
}

mjpeg_player_state_t mjpeg_player_get_state(mjpeg_player_handle_t handle)
{
    if (handle == NULL) {
        return MJPEG_STATE_ERROR;
    }

    return handle->state;
}

void mjpeg_player_get_progress(mjpeg_player_handle_t handle, uint32_t *current_frame, uint32_t *total_frames)
{
    if (handle == NULL) {
        if (current_frame) *current_frame = 0;
        if (total_frames) *total_frames = 0;
        return;
    }

    if (current_frame) *current_frame = handle->current_frame;
    if (total_frames) *total_frames = handle->frame_count;
}
