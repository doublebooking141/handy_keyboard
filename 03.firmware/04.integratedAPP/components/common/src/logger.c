/**
 * @file logger.c
 * @brief Handy Keyboard - ログ出力ラッパー実装
 */

#include "logger.h"
#include <string.h>

static const char *TAG = "LOGGER";

esp_err_t logger_init(void)
{
    // デフォルトログレベル設定（sdkconfig.defaultsで制御）
    ESP_LOGI(TAG, "Logger initialized");
    ESP_LOGI(TAG, "Default log level: INFO");

    return ESP_OK;
}

void logger_set_level(const char* tag, esp_log_level_t level)
{
    esp_log_level_set(tag, level);
    ESP_LOGI(TAG, "Set log level for '%s' to %s",
             tag, logger_level_to_string(level));
}

const char* logger_level_to_string(esp_log_level_t level)
{
    switch (level) {
        case ESP_LOG_NONE:    return "NONE";
        case ESP_LOG_ERROR:   return "ERROR";
        case ESP_LOG_WARN:    return "WARN";
        case ESP_LOG_INFO:    return "INFO";
        case ESP_LOG_DEBUG:   return "DEBUG";
        case ESP_LOG_VERBOSE: return "VERBOSE";
        default:              return "UNKNOWN";
    }
}
