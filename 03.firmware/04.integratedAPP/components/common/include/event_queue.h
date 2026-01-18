/**
 * @file event_queue.h
 * @brief Handy Keyboard - イベントキューシステム
 *
 * タッチパッド、キーボード、マウス、システムイベントの管理
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// キューサイズ定義
#define EVENT_QUEUE_SIZE_INPUT      32    ///< 入力イベント（タッチ、物理キー）
#define EVENT_QUEUE_SIZE_HID        16    ///< HID レポート（USB/BLE出力）
#define EVENT_QUEUE_SIZE_SYSTEM     8     ///< システムイベント（設定変更等）

/**
 * @brief キュー送信ポリシー
 */
typedef enum {
    QUEUE_POLICY_NO_WAIT,      ///< すぐに失敗（ノンブロッキング）
    QUEUE_POLICY_BLOCK_10MS,   ///< 10ms まで待機
    QUEUE_POLICY_BLOCK_50MS,   ///< 50ms まで待機
    QUEUE_POLICY_BLOCK_100MS,  ///< 100ms まで待機
} queue_send_policy_t;

/**
 * @brief 入力イベント種別
 */
typedef enum {
    EVENT_TOUCH_TAP,           ///< タップ（クリック相当）
    EVENT_TOUCH_DOUBLE_TAP,    ///< ダブルタップ
    EVENT_TOUCH_FLICK,         ///< フリック入力
    EVENT_TOUCH_TWO_FINGER_SCROLL,  ///< 2本指スクロール
    EVENT_TOUCH_HOLD,          ///< 長押し
    EVENT_KEY_PRESS,           ///< キー押下
    EVENT_KEY_RELEASE,         ///< キー離上
    EVENT_SYSTEM_MODE_CHANGE,  ///< モード変更（キーボード/マウス/時計）
    EVENT_SYSTEM_SETTING_CHANGE,  ///< 設定変更
} input_event_type_t;

/**
 * @brief タッチイベントデータ
 */
typedef struct {
    uint16_t x;                ///< X座標
    uint16_t y;                ///< Y座標
    uint8_t finger_count;      ///< 指の本数（マルチタッチ）
    int16_t flick_direction;   ///< フリック方向（0=上, 90=右, 180=下, 270=左）
    int8_t scroll_x;           ///< スクロール量 X
    int8_t scroll_y;           ///< スクロール量 Y
} touch_event_data_t;

/**
 * @brief キーイベントデータ
 */
typedef struct {
    uint16_t keycode;          ///< キーコード
    uint8_t modifiers;         ///< モディファイア（Ctrl, Shift等）
} key_event_data_t;

/**
 * @brief システムイベントデータ
 */
typedef struct {
    uint8_t mode;              ///< モード番号
    uint8_t param;             ///< パラメータ
} system_event_data_t;

/**
 * @brief 入力イベント統合構造体
 */
typedef struct {
    input_event_type_t type;   ///< イベント種別
    uint32_t timestamp;        ///< タイムスタンプ（ms）
    union {
        touch_event_data_t touch;
        key_event_data_t key;
        system_event_data_t system;
    } data;
} input_event_t;

/**
 * @brief HID レポート種別
 */
typedef enum {
    HID_REPORT_KEYBOARD,       ///< キーボードレポート
    HID_REPORT_MOUSE,          ///< マウスレポート
    HID_REPORT_CONSUMER,       ///< コンシューマーコントロール
} hid_report_type_t;

/**
 * @brief キーボードレポート（6KRO Boot Protocol）
 */
typedef struct {
    uint8_t modifiers;         ///< モディファイア（Ctrl, Shift等）
    uint8_t reserved;          ///< 予約
    uint8_t keys[6];           ///< 同時押しキー（最大6個）
} kbd_report_t;

/**
 * @brief マウスレポート
 */
typedef struct {
    uint8_t buttons;           ///< ボタン状態（bit0=左, bit1=右, bit2=中央）
    int8_t x;                  ///< X軸移動量
    int8_t y;                  ///< Y軸移動量
    int8_t wheel;              ///< ホイール量
} mouse_report_t;

/**
 * @brief コンシューマーコントロールレポート
 */
typedef struct {
    uint16_t usage;            ///< Usage ID（音量、再生等）
} consumer_report_t;

/**
 * @brief HID レポート統合構造体
 */
typedef struct {
    hid_report_type_t type;    ///< レポート種別
    union {
        kbd_report_t keyboard;
        mouse_report_t mouse;
        consumer_report_t consumer;
    } data;
} hid_report_t;

/**
 * @brief イベントキューシステム初期化
 *
 * 各種イベントキューを作成
 *
 * @return ESP_OK 成功
 */
esp_err_t event_queue_init(void);

/**
 * @brief 入力イベント送信
 *
 * @param event イベントデータ
 * @param policy 送信ポリシー
 * @return true 成功, false 失敗
 */
bool input_event_send(const input_event_t *event, queue_send_policy_t policy);

/**
 * @brief 入力イベント受信
 *
 * @param event イベントデータ格納先
 * @param wait_ticks 待機時間（FreeRTOS ticks）
 * @return true 受信成功, false タイムアウト
 */
bool input_event_receive(input_event_t *event, TickType_t wait_ticks);

/**
 * @brief HID レポート送信
 *
 * @param report レポートデータ
 * @param policy 送信ポリシー
 * @return true 成功, false 失敗
 */
bool hid_report_send(const hid_report_t *report, queue_send_policy_t policy);

/**
 * @brief HID レポート受信
 *
 * @param report レポートデータ格納先
 * @param wait_ticks 待機時間（FreeRTOS ticks）
 * @return true 受信成功, false タイムアウト
 */
bool hid_report_receive(hid_report_t *report, TickType_t wait_ticks);

/**
 * @brief システムイベント送信
 *
 * @param event イベントデータ
 * @param policy 送信ポリシー
 * @return true 成功, false 失敗
 */
bool system_event_send(const input_event_t *event, queue_send_policy_t policy);

/**
 * @brief システムイベント受信
 *
 * @param event イベントデータ格納先
 * @param wait_ticks 待機時間（FreeRTOS ticks）
 * @return true 受信成功, false タイムアウト
 */
bool system_event_receive(input_event_t *event, TickType_t wait_ticks);

/**
 * @brief 入力イベントキューの待機数取得
 *
 * @return 待機中のイベント数
 */
size_t input_event_queue_waiting(void);

/**
 * @brief HID レポートキューの待機数取得
 *
 * @return 待機中のレポート数
 */
size_t hid_report_queue_waiting(void);

#ifdef __cplusplus
}
#endif
