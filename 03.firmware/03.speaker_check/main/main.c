#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal_audio.h"
#include "board_config.h"

static const char *TAG = "speaker_test";

static void play_sweep() {
    size_t bytes_write = 0;
    int16_t samples[1024];
    float phase = 0;
    float freq = 200.0f; 
    float freq_end = 2000.0f;
    float freq_step = 10.0f;

    ESP_LOGI(TAG, "Starting Sweep...");
    while (freq < freq_end) {
        float increment = (2.0f * M_PI * freq) / AUDIO_SAMPLE_RATE;
        for (int i = 0; i < 1024; i += 2) {
            int16_t val = (int16_t)(sin(phase) * 4000.0f); // Amplitude 4000 (Safe)
            samples[i] = val;
            samples[i+1] = val;
            phase += increment;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
        hal_audio_write_i2s(samples, sizeof(samples), &bytes_write, 1000 / portTICK_PERIOD_MS);
        freq += freq_step;
        // vTaskDelay removed to prevent buffer underflow
    }
    ESP_LOGI(TAG, "Sweep Done");
}

void app_main(void) {
    ESP_LOGI(TAG, "Speaker Check Start");
    hal_audio_init();

    while (1) {
        play_sweep();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}