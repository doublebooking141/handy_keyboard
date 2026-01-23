/**
 * @file audio_player.c
 * @brief Audio Player Implementation - WAV file playback via ES8311 codec
 */

#include "audio_player.h"
#include "bsp/jc4880p443c.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "AUDIO_PLAYER";

// Task configuration
#define AUDIO_TASK_STACK_SIZE  8192
#define AUDIO_TASK_PRIORITY    6
#define AUDIO_BUFFER_SIZE      4096  // PCM buffer size per read

// WAV file header structures
#pragma pack(push, 1)
typedef struct {
    char     riff_tag[4];      // "RIFF"
    uint32_t file_size;        // File size - 8
    char     wave_tag[4];      // "WAVE"
} wav_riff_header_t;

typedef struct {
    char     chunk_id[4];      // "fmt " or "data"
    uint32_t chunk_size;       // Chunk size
} wav_chunk_header_t;

typedef struct {
    uint16_t audio_format;     // 1 = PCM
    uint16_t num_channels;     // 1 = mono, 2 = stereo
    uint32_t sample_rate;      // 8000, 16000, 22050, 44100, 48000
    uint32_t byte_rate;        // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;      // num_channels * bits_per_sample/8
    uint16_t bits_per_sample;  // 8 or 16
} wav_fmt_chunk_t;
#pragma pack(pop)

// Player state
static struct {
    // Codec handle
    esp_codec_dev_handle_t codec;
    bool codec_initialized;
    bool codec_opened;

    // Playback state
    audio_player_state_t state;
    uint8_t volume;

    // Current file info
    FILE *file;
    char file_path[128];
    bool loop;
    uint32_t data_offset;      // Start of PCM data in file
    uint32_t data_size;        // Size of PCM data
    uint32_t bytes_played;     // Bytes played so far

    // Audio format
    uint16_t sample_rate;
    uint8_t  channels;
    uint8_t  bits_per_sample;

    // Task management
    TaskHandle_t task_handle;
    SemaphoreHandle_t mutex;
    volatile bool task_running;
    volatile bool stop_requested;

    // Callback
    audio_player_callback_t callback;
    void *callback_user_data;

    // PCM buffer (DMA capable)
    uint8_t *pcm_buffer;
} s_player = {
    .state = AUDIO_PLAYER_STATE_IDLE,
    .volume = 80,
};

/**
 * @brief Parse WAV file header and find PCM data
 */
static esp_err_t parse_wav_header(FILE *file)
{
    wav_riff_header_t riff;
    wav_chunk_header_t chunk;
    wav_fmt_chunk_t fmt;
    bool fmt_found = false;
    bool data_found = false;

    // Read RIFF header
    if (fread(&riff, sizeof(riff), 1, file) != 1) {
        ESP_LOGE(TAG, "Failed to read RIFF header");
        return ESP_ERR_INVALID_SIZE;
    }

    // Validate RIFF/WAVE
    if (memcmp(riff.riff_tag, "RIFF", 4) != 0 ||
        memcmp(riff.wave_tag, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "Not a valid WAV file");
        return ESP_ERR_INVALID_ARG;
    }

    // Parse chunks
    while (!data_found && fread(&chunk, sizeof(chunk), 1, file) == 1) {
        if (memcmp(chunk.chunk_id, "fmt ", 4) == 0) {
            // Format chunk
            if (chunk.chunk_size < sizeof(fmt)) {
                ESP_LOGE(TAG, "Invalid fmt chunk size");
                return ESP_ERR_INVALID_SIZE;
            }

            if (fread(&fmt, sizeof(fmt), 1, file) != 1) {
                ESP_LOGE(TAG, "Failed to read fmt chunk");
                return ESP_ERR_INVALID_SIZE;
            }

            // Skip extra fmt data if any
            if (chunk.chunk_size > sizeof(fmt)) {
                fseek(file, chunk.chunk_size - sizeof(fmt), SEEK_CUR);
            }

            // Validate format
            if (fmt.audio_format != 1) {
                ESP_LOGE(TAG, "Unsupported audio format: %d (only PCM supported)", fmt.audio_format);
                return ESP_ERR_NOT_SUPPORTED;
            }

            if (fmt.bits_per_sample != 8 && fmt.bits_per_sample != 16) {
                ESP_LOGE(TAG, "Unsupported bits per sample: %d", fmt.bits_per_sample);
                return ESP_ERR_NOT_SUPPORTED;
            }

            if (fmt.num_channels != 1 && fmt.num_channels != 2) {
                ESP_LOGE(TAG, "Unsupported channels: %d", fmt.num_channels);
                return ESP_ERR_NOT_SUPPORTED;
            }

            s_player.sample_rate = fmt.sample_rate;
            s_player.channels = fmt.num_channels;
            s_player.bits_per_sample = fmt.bits_per_sample;
            fmt_found = true;

            ESP_LOGI(TAG, "WAV format: %luHz, %d-bit, %s",
                     (unsigned long)fmt.sample_rate,
                     fmt.bits_per_sample,
                     fmt.num_channels == 1 ? "mono" : "stereo");

        } else if (memcmp(chunk.chunk_id, "data", 4) == 0) {
            // Data chunk
            if (!fmt_found) {
                ESP_LOGE(TAG, "Data chunk before fmt chunk");
                return ESP_ERR_INVALID_STATE;
            }

            s_player.data_offset = ftell(file);
            s_player.data_size = chunk.chunk_size;
            data_found = true;

            ESP_LOGI(TAG, "WAV data: %lu bytes at offset %lu",
                     (unsigned long)s_player.data_size,
                     (unsigned long)s_player.data_offset);

        } else {
            // Skip unknown chunk
            fseek(file, chunk.chunk_size, SEEK_CUR);
        }
    }

    if (!fmt_found || !data_found) {
        ESP_LOGE(TAG, "WAV file missing required chunks");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/**
 * @brief Configure codec for current audio format
 */
static esp_err_t configure_codec(void)
{
    if (!s_player.codec) {
        return ESP_ERR_INVALID_STATE;
    }

    // Close previous session if open
    if (s_player.codec_opened) {
        esp_codec_dev_close(s_player.codec);
        s_player.codec_opened = false;
    }

    // Configure sample info
    esp_codec_dev_sample_info_t sample_info = {
        .sample_rate = s_player.sample_rate,
        .channel = s_player.channels,
        .bits_per_sample = s_player.bits_per_sample,
    };

    esp_err_t ret = esp_codec_dev_open(s_player.codec, &sample_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open codec: %s", esp_err_to_name(ret));
        return ret;
    }
    s_player.codec_opened = true;

    // Set volume
    ret = esp_codec_dev_set_out_vol(s_player.codec, s_player.volume);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set volume: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

/**
 * @brief Audio playback task
 */
static void audio_playback_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Playback task started");

    while (s_player.task_running) {
        if (s_player.state == AUDIO_PLAYER_STATE_PLAYING && s_player.file) {
            // Read PCM data
            size_t bytes_to_read = AUDIO_BUFFER_SIZE;
            uint32_t remaining = s_player.data_size - s_player.bytes_played;

            if (remaining < bytes_to_read) {
                bytes_to_read = remaining;
            }

            if (bytes_to_read > 0) {
                size_t bytes_read = fread(s_player.pcm_buffer, 1, bytes_to_read, s_player.file);

                if (bytes_read > 0) {
                    // Write to codec
                    esp_err_t ret = esp_codec_dev_write(s_player.codec, s_player.pcm_buffer, bytes_read);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Codec write failed: %s", esp_err_to_name(ret));
                        s_player.state = AUDIO_PLAYER_STATE_ERROR;
                        continue;
                    }
                    s_player.bytes_played += bytes_read;
                }
            }

            // Check for end of file
            if (s_player.bytes_played >= s_player.data_size) {
                if (s_player.loop && !s_player.stop_requested) {
                    // Loop: seek back to start of data
                    fseek(s_player.file, s_player.data_offset, SEEK_SET);
                    s_player.bytes_played = 0;
                    ESP_LOGD(TAG, "Looping playback");
                } else {
                    // Playback complete
                    ESP_LOGI(TAG, "Playback complete");
                    s_player.state = AUDIO_PLAYER_STATE_IDLE;

                    // Close file
                    if (s_player.file) {
                        fclose(s_player.file);
                        s_player.file = NULL;
                    }

                    // Invoke callback
                    if (s_player.callback) {
                        s_player.callback(s_player.callback_user_data, true);
                    }
                }
            }

            // Check for stop request
            if (s_player.stop_requested) {
                ESP_LOGI(TAG, "Stop requested");
                s_player.state = AUDIO_PLAYER_STATE_IDLE;
                s_player.stop_requested = false;

                if (s_player.file) {
                    fclose(s_player.file);
                    s_player.file = NULL;
                }

                if (s_player.callback) {
                    s_player.callback(s_player.callback_user_data, false);
                }
            }
        } else if (s_player.state == AUDIO_PLAYER_STATE_PAUSED) {
            // Paused - just wait
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            // Idle - wait for new playback
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    ESP_LOGI(TAG, "Playback task stopped");
    vTaskDelete(NULL);
}

esp_err_t audio_player_init(void)
{
    if (s_player.codec_initialized) {
        ESP_LOGW(TAG, "Audio player already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing audio player...");

    // Create mutex
    s_player.mutex = xSemaphoreCreateMutex();
    if (!s_player.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Allocate PCM buffer (prefer PSRAM)
    s_player.pcm_buffer = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_player.pcm_buffer) {
        s_player.pcm_buffer = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_DEFAULT);
    }
    if (!s_player.pcm_buffer) {
        ESP_LOGE(TAG, "Failed to allocate PCM buffer");
        vSemaphoreDelete(s_player.mutex);
        return ESP_ERR_NO_MEM;
    }

    // Initialize codec via BSP
    s_player.codec = bsp_audio_codec_speaker_init();
    if (!s_player.codec) {
        ESP_LOGE(TAG, "Failed to initialize audio codec");
        free(s_player.pcm_buffer);
        vSemaphoreDelete(s_player.mutex);
        return ESP_FAIL;
    }

    s_player.codec_initialized = true;
    s_player.state = AUDIO_PLAYER_STATE_IDLE;

    // Create playback task
    s_player.task_running = true;
    BaseType_t ret = xTaskCreate(
        audio_playback_task,
        "audio_play",
        AUDIO_TASK_STACK_SIZE,
        NULL,
        AUDIO_TASK_PRIORITY,
        &s_player.task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create playback task");
        s_player.task_running = false;
        free(s_player.pcm_buffer);
        vSemaphoreDelete(s_player.mutex);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Audio player initialized");
    return ESP_OK;
}

void audio_player_deinit(void)
{
    if (!s_player.codec_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing audio player...");

    // Stop task
    s_player.task_running = false;
    s_player.stop_requested = true;
    vTaskDelay(pdMS_TO_TICKS(200));

    // Close file if open
    if (s_player.file) {
        fclose(s_player.file);
        s_player.file = NULL;
    }

    // Close codec
    if (s_player.codec_opened) {
        esp_codec_dev_close(s_player.codec);
        s_player.codec_opened = false;
    }

    // Free resources
    if (s_player.pcm_buffer) {
        free(s_player.pcm_buffer);
        s_player.pcm_buffer = NULL;
    }

    if (s_player.mutex) {
        vSemaphoreDelete(s_player.mutex);
        s_player.mutex = NULL;
    }

    s_player.codec_initialized = false;
    s_player.state = AUDIO_PLAYER_STATE_IDLE;

    ESP_LOGI(TAG, "Audio player deinitialized");
}

esp_err_t audio_player_play_file(const char *file_path, uint8_t volume, bool loop)
{
    if (!s_player.codec_initialized) {
        ESP_LOGE(TAG, "Audio player not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!file_path || strlen(file_path) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_player.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Stop any current playback
    if (s_player.state == AUDIO_PLAYER_STATE_PLAYING) {
        s_player.stop_requested = true;
        xSemaphoreGive(s_player.mutex);
        vTaskDelay(pdMS_TO_TICKS(100));
        if (xSemaphoreTake(s_player.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }

    // Open file
    s_player.file = fopen(file_path, "rb");
    if (!s_player.file) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        xSemaphoreGive(s_player.mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Parse WAV header
    esp_err_t ret = parse_wav_header(s_player.file);
    if (ret != ESP_OK) {
        fclose(s_player.file);
        s_player.file = NULL;
        xSemaphoreGive(s_player.mutex);
        return ret;
    }

    // Configure codec
    ret = configure_codec();
    if (ret != ESP_OK) {
        fclose(s_player.file);
        s_player.file = NULL;
        xSemaphoreGive(s_player.mutex);
        return ret;
    }

    // Set volume
    s_player.volume = volume > 100 ? 100 : volume;
    esp_codec_dev_set_out_vol(s_player.codec, s_player.volume);

    // Store settings
    strncpy(s_player.file_path, file_path, sizeof(s_player.file_path) - 1);
    s_player.loop = loop;
    s_player.bytes_played = 0;
    s_player.stop_requested = false;

    // Seek to data start
    fseek(s_player.file, s_player.data_offset, SEEK_SET);

    // Start playback
    s_player.state = AUDIO_PLAYER_STATE_PLAYING;

    ESP_LOGI(TAG, "Playing: %s (vol=%d, loop=%d)", file_path, volume, loop);

    xSemaphoreGive(s_player.mutex);
    return ESP_OK;
}

esp_err_t audio_player_stop(void)
{
    if (!s_player.codec_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_player.stop_requested = true;

    // Wait for task to process stop
    for (int i = 0; i < 20 && s_player.state == AUDIO_PLAYER_STATE_PLAYING; i++) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return ESP_OK;
}

esp_err_t audio_player_pause(void)
{
    if (!s_player.codec_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_player.state == AUDIO_PLAYER_STATE_PLAYING) {
        s_player.state = AUDIO_PLAYER_STATE_PAUSED;
        ESP_LOGI(TAG, "Playback paused");
    }

    return ESP_OK;
}

esp_err_t audio_player_resume(void)
{
    if (!s_player.codec_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_player.state == AUDIO_PLAYER_STATE_PAUSED) {
        s_player.state = AUDIO_PLAYER_STATE_PLAYING;
        ESP_LOGI(TAG, "Playback resumed");
    }

    return ESP_OK;
}

esp_err_t audio_player_set_volume(uint8_t volume)
{
    if (!s_player.codec_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_player.volume = volume > 100 ? 100 : volume;

    if (s_player.codec_opened) {
        return esp_codec_dev_set_out_vol(s_player.codec, s_player.volume);
    }

    return ESP_OK;
}

uint8_t audio_player_get_volume(void)
{
    return s_player.volume;
}

bool audio_player_is_playing(void)
{
    return s_player.state == AUDIO_PLAYER_STATE_PLAYING;
}

audio_player_state_t audio_player_get_state(void)
{
    return s_player.state;
}

esp_err_t audio_player_register_callback(audio_player_callback_t callback, void *user_data)
{
    s_player.callback = callback;
    s_player.callback_user_data = user_data;
    return ESP_OK;
}
