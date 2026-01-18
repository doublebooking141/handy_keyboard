/**
 * @file logger.h
 * @brief Handy Keyboard - ログ出力ラッパー
 *
 * ESP-IDF の esp_log.h を拡張したロギングインフラ
 */

#pragma once

#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ロガー初期化
 *
 * ログレベルの設定、出力先の初期化を行う
 *
 * @return ESP_OK 成功
 */
esp_err_t logger_init(void);

/**
 * @brief コンポーネント別ログレベル設定
 *
 * @param tag コンポーネントタグ
 * @param level ログレベル (ESP_LOG_NONE, ESP_LOG_ERROR, ESP_LOG_WARN, ESP_LOG_INFO, ESP_LOG_DEBUG, ESP_LOG_VERBOSE)
 */
void logger_set_level(const char* tag, esp_log_level_t level);

/**
 * @brief ログレベル文字列取得
 *
 * @param level ログレベル
 * @return レベル文字列 ("ERROR", "WARN", "INFO", "DEBUG", "VERBOSE")
 */
const char* logger_level_to_string(esp_log_level_t level);

// 便利マクロ（コンポーネントタグを自動挿入）
#define LOG_E(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define LOG_V(tag, format, ...) ESP_LOGV(tag, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
