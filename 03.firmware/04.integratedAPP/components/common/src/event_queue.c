/**
 * @file event_queue.c
 * @brief Handy Keyboard - イベントキューシステム実装
 */

#include "event_queue.h"
#include "logger.h"
#include <string.h>

static const char *TAG = "EVENT_QUEUE";

// イベントキューハンドル
static QueueHandle_t input_event_queue = NULL;
static QueueHandle_t hid_report_queue = NULL;
static QueueHandle_t system_event_queue = NULL;

/**
 * @brief ポリシーに応じた待機時間取得
 */
static TickType_t get_wait_ticks(queue_send_policy_t policy)
{
    switch (policy) {
        case QUEUE_POLICY_NO_WAIT:
            return 0;
        case QUEUE_POLICY_BLOCK_10MS:
            return pdMS_TO_TICKS(10);
        case QUEUE_POLICY_BLOCK_50MS:
            return pdMS_TO_TICKS(50);
        case QUEUE_POLICY_BLOCK_100MS:
            return pdMS_TO_TICKS(100);
        default:
            return 0;
    }
}

esp_err_t event_queue_init(void)
{
    ESP_LOGI(TAG, "Initializing event queue system");

    // 入力イベントキュー作成
    input_event_queue = xQueueCreate(EVENT_QUEUE_SIZE_INPUT, sizeof(input_event_t));
    if (input_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create input event queue");
        return ESP_ERR_NO_MEM;
    }

    // HID レポートキュー作成
    hid_report_queue = xQueueCreate(EVENT_QUEUE_SIZE_HID, sizeof(hid_report_t));
    if (hid_report_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create HID report queue");
        return ESP_ERR_NO_MEM;
    }

    // システムイベントキュー作成
    system_event_queue = xQueueCreate(EVENT_QUEUE_SIZE_SYSTEM, sizeof(input_event_t));
    if (system_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create system event queue");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Event queues created successfully");
    ESP_LOGI(TAG, "  Input queue: %d slots", EVENT_QUEUE_SIZE_INPUT);
    ESP_LOGI(TAG, "  HID queue: %d slots", EVENT_QUEUE_SIZE_HID);
    ESP_LOGI(TAG, "  System queue: %d slots", EVENT_QUEUE_SIZE_SYSTEM);

    return ESP_OK;
}

bool input_event_send(const input_event_t *event, queue_send_policy_t policy)
{
    if (input_event_queue == NULL || event == NULL) {
        return false;
    }

    TickType_t wait = get_wait_ticks(policy);
    BaseType_t result = xQueueSend(input_event_queue, event, wait);

    if (result != pdTRUE) {
        ESP_LOGW(TAG, "Input event queue full (policy: %d)", policy);
        return false;
    }

    return true;
}

bool input_event_receive(input_event_t *event, TickType_t wait_ticks)
{
    if (input_event_queue == NULL || event == NULL) {
        return false;
    }

    return xQueueReceive(input_event_queue, event, wait_ticks) == pdTRUE;
}

bool hid_report_send(const hid_report_t *report, queue_send_policy_t policy)
{
    if (hid_report_queue == NULL || report == NULL) {
        return false;
    }

    TickType_t wait = get_wait_ticks(policy);
    BaseType_t result = xQueueSend(hid_report_queue, report, wait);

    if (result != pdTRUE) {
        ESP_LOGW(TAG, "HID report queue full (policy: %d)", policy);
        return false;
    }

    return true;
}

bool hid_report_receive(hid_report_t *report, TickType_t wait_ticks)
{
    if (hid_report_queue == NULL || report == NULL) {
        return false;
    }

    return xQueueReceive(hid_report_queue, report, wait_ticks) == pdTRUE;
}

bool system_event_send(const input_event_t *event, queue_send_policy_t policy)
{
    if (system_event_queue == NULL || event == NULL) {
        return false;
    }

    TickType_t wait = get_wait_ticks(policy);
    BaseType_t result = xQueueSend(system_event_queue, event, wait);

    if (result != pdTRUE) {
        ESP_LOGW(TAG, "System event queue full (policy: %d)", policy);
        return false;
    }

    return true;
}

bool system_event_receive(input_event_t *event, TickType_t wait_ticks)
{
    if (system_event_queue == NULL || event == NULL) {
        return false;
    }

    return xQueueReceive(system_event_queue, event, wait_ticks) == pdTRUE;
}

size_t input_event_queue_waiting(void)
{
    if (input_event_queue == NULL) {
        return 0;
    }
    return (size_t)uxQueueMessagesWaiting(input_event_queue);
}

size_t hid_report_queue_waiting(void)
{
    if (hid_report_queue == NULL) {
        return 0;
    }
    return (size_t)uxQueueMessagesWaiting(hid_report_queue);
}
