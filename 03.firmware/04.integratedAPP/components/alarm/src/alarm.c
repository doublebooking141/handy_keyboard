/**
 * @file alarm.c
 * @brief Alarm Management Implementation
 */

#include "alarm.h"
#include "audio_player.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "ALARM";

// NVS configuration
#define NVS_NAMESPACE    "alarm_cfg"
#define NVS_KEY_PREFIX   "alarm_"

// Alarm volume
#define ALARM_VOLUME     85

// Auto-dismiss timeout (minutes)
#define ALARM_AUTO_DISMISS_MINUTES 5

// Alarm state
static struct {
    alarm_entry_t alarms[ALARM_MAX_COUNT];
    nvs_handle_t nvs_handle;
    bool nvs_opened;
    bool initialized;

    // Triggered state
    bool triggered;
    uint8_t triggered_index;
    time_t triggered_time;

    // Snooze state
    bool snooze_active;
    uint8_t snooze_index;
    time_t snooze_until;

    // Callback
    alarm_event_cb_t callback;
    void *callback_user_data;

    // Thread safety
    SemaphoreHandle_t mutex;

    // Last checked minute (to avoid duplicate triggers)
    int last_checked_minute;
    int last_checked_hour;
} s_alarm = {
    .triggered_index = 0xFF,
    .snooze_index = 0xFF,
    .last_checked_minute = -1,
    .last_checked_hour = -1,
};

/**
 * @brief Generate NVS key for alarm index
 */
static void get_nvs_key(uint8_t index, char *key, size_t key_len)
{
    snprintf(key, key_len, "%s%d", NVS_KEY_PREFIX, index);
}

/**
 * @brief Load single alarm from NVS
 */
static esp_err_t load_alarm(uint8_t index)
{
    if (!s_alarm.nvs_opened) {
        return ESP_ERR_INVALID_STATE;
    }

    char key[16];
    get_nvs_key(index, key, sizeof(key));

    size_t size = sizeof(alarm_entry_t);
    esp_err_t ret = nvs_get_blob(s_alarm.nvs_handle, key, &s_alarm.alarms[index], &size);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Initialize with defaults
        memset(&s_alarm.alarms[index], 0, sizeof(alarm_entry_t));
        s_alarm.alarms[index].hour = 7;
        s_alarm.alarms[index].minute = 0;
        s_alarm.alarms[index].enabled = false;
        return ESP_OK;
    }

    return ret;
}

/**
 * @brief Save single alarm to NVS
 */
static esp_err_t save_alarm(uint8_t index)
{
    if (!s_alarm.nvs_opened) {
        return ESP_ERR_INVALID_STATE;
    }

    char key[16];
    get_nvs_key(index, key, sizeof(key));

    esp_err_t ret = nvs_set_blob(s_alarm.nvs_handle, key, &s_alarm.alarms[index], sizeof(alarm_entry_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(s_alarm.nvs_handle);
    }

    return ret;
}

/**
 * @brief Check if sound file exists
 */
static bool sound_file_exists(const char *path)
{
    if (!path || path[0] == '\0') {
        return false;
    }

    struct stat st;
    return (stat(path, &st) == 0);
}

/**
 * @brief Get sound path for alarm (use default if not set or not found)
 */
static const char* get_sound_path(uint8_t index)
{
    const char *path = s_alarm.alarms[index].sound_path;

    if (path[0] != '\0' && sound_file_exists(path)) {
        return path;
    }

    // Try default sound
    if (sound_file_exists(ALARM_DEFAULT_SOUND)) {
        return ALARM_DEFAULT_SOUND;
    }

    return NULL;
}

/**
 * @brief Trigger alarm playback
 */
static void trigger_alarm(uint8_t index)
{
    const char *sound_path = get_sound_path(index);

    ESP_LOGI(TAG, "Alarm %d triggered at %02d:%02d",
             index, s_alarm.alarms[index].hour, s_alarm.alarms[index].minute);

    s_alarm.triggered = true;
    s_alarm.triggered_index = index;
    s_alarm.triggered_time = time(NULL);
    s_alarm.snooze_active = false;

    // Play sound
    if (sound_path) {
        esp_err_t ret = audio_player_play_file(sound_path, ALARM_VOLUME, true);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to play alarm sound: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "No alarm sound file available");
    }

    // Invoke callback
    if (s_alarm.callback) {
        s_alarm.callback(ALARM_EVENT_TRIGGERED, index, s_alarm.callback_user_data);
    }

    // Disable one-time alarms
    if (s_alarm.alarms[index].repeat_days == ALARM_REPEAT_ONCE) {
        s_alarm.alarms[index].enabled = false;
        save_alarm(index);
    }
}

/**
 * @brief Audio playback completion callback
 */
static void audio_callback(void *user_data, bool completed)
{
    (void)user_data;
    // If playback completed (not stopped), alarm expired
    if (completed && s_alarm.triggered) {
        ESP_LOGI(TAG, "Alarm audio completed");
    }
}

esp_err_t alarm_init(void)
{
    if (s_alarm.initialized) {
        ESP_LOGW(TAG, "Alarm already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing alarm system...");

    // Create mutex
    s_alarm.mutex = xSemaphoreCreateMutex();
    if (!s_alarm.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Open NVS namespace
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_alarm.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        vSemaphoreDelete(s_alarm.mutex);
        return ret;
    }
    s_alarm.nvs_opened = true;

    // Load all alarms
    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        load_alarm(i);
        if (s_alarm.alarms[i].enabled) {
            ESP_LOGI(TAG, "Alarm %d: %02d:%02d (repeat=0x%02X)",
                     i, s_alarm.alarms[i].hour, s_alarm.alarms[i].minute,
                     s_alarm.alarms[i].repeat_days);
        }
    }

    // Initialize audio player if not already
    ret = audio_player_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Audio player init: %s", esp_err_to_name(ret));
    }

    // Register audio callback
    audio_player_register_callback(audio_callback, NULL);

    s_alarm.initialized = true;
    ESP_LOGI(TAG, "Alarm system initialized (%d alarms enabled)", alarm_get_enabled_count());

    return ESP_OK;
}

void alarm_deinit(void)
{
    if (!s_alarm.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing alarm system...");

    // Stop any playing alarm
    if (s_alarm.triggered) {
        audio_player_stop();
    }

    // Close NVS
    if (s_alarm.nvs_opened) {
        nvs_close(s_alarm.nvs_handle);
        s_alarm.nvs_opened = false;
    }

    if (s_alarm.mutex) {
        vSemaphoreDelete(s_alarm.mutex);
        s_alarm.mutex = NULL;
    }

    s_alarm.initialized = false;
    ESP_LOGI(TAG, "Alarm system deinitialized");
}

esp_err_t alarm_get(uint8_t index, alarm_entry_t *alarm)
{
    if (index >= ALARM_MAX_COUNT || !alarm) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_alarm.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    memcpy(alarm, &s_alarm.alarms[index], sizeof(alarm_entry_t));

    xSemaphoreGive(s_alarm.mutex);
    return ESP_OK;
}

esp_err_t alarm_set(uint8_t index, const alarm_entry_t *alarm)
{
    if (index >= ALARM_MAX_COUNT || !alarm) {
        return ESP_ERR_INVALID_ARG;
    }

    if (alarm->hour > 23 || alarm->minute > 59) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_alarm.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    memcpy(&s_alarm.alarms[index], alarm, sizeof(alarm_entry_t));
    esp_err_t ret = save_alarm(index);

    ESP_LOGI(TAG, "Alarm %d set: %02d:%02d, repeat=0x%02X, enabled=%d",
             index, alarm->hour, alarm->minute, alarm->repeat_days, alarm->enabled);

    xSemaphoreGive(s_alarm.mutex);
    return ret;
}

esp_err_t alarm_delete(uint8_t index)
{
    if (index >= ALARM_MAX_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_alarm.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    memset(&s_alarm.alarms[index], 0, sizeof(alarm_entry_t));
    s_alarm.alarms[index].hour = 7;
    s_alarm.alarms[index].enabled = false;

    esp_err_t ret = save_alarm(index);

    ESP_LOGI(TAG, "Alarm %d deleted", index);

    xSemaphoreGive(s_alarm.mutex);
    return ret;
}

uint8_t alarm_get_enabled_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        if (s_alarm.alarms[i].enabled) {
            count++;
        }
    }
    return count;
}

bool alarm_is_triggered(void)
{
    return s_alarm.triggered;
}

uint8_t alarm_get_triggered_index(void)
{
    return s_alarm.triggered_index;
}

esp_err_t alarm_dismiss(void)
{
    if (!s_alarm.triggered) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Alarm %d dismissed", s_alarm.triggered_index);

    audio_player_stop();

    uint8_t index = s_alarm.triggered_index;
    s_alarm.triggered = false;
    s_alarm.triggered_index = 0xFF;
    s_alarm.snooze_active = false;

    if (s_alarm.callback) {
        s_alarm.callback(ALARM_EVENT_DISMISSED, index, s_alarm.callback_user_data);
    }

    return ESP_OK;
}

esp_err_t alarm_snooze(void)
{
    if (!s_alarm.triggered) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Alarm %d snoozed for %d minutes",
             s_alarm.triggered_index, ALARM_SNOOZE_MINUTES);

    audio_player_stop();

    uint8_t index = s_alarm.triggered_index;
    s_alarm.triggered = false;
    s_alarm.triggered_index = 0xFF;

    // Set snooze
    s_alarm.snooze_active = true;
    s_alarm.snooze_index = index;
    s_alarm.snooze_until = time(NULL) + (ALARM_SNOOZE_MINUTES * 60);

    if (s_alarm.callback) {
        s_alarm.callback(ALARM_EVENT_SNOOZED, index, s_alarm.callback_user_data);
    }

    return ESP_OK;
}

void alarm_check_time(const struct tm *now)
{
    if (!s_alarm.initialized || !now) {
        return;
    }

    // Only check once per minute
    if (now->tm_hour == s_alarm.last_checked_hour &&
        now->tm_min == s_alarm.last_checked_minute) {
        // Still check for snooze and auto-dismiss
        goto check_snooze;
    }

    s_alarm.last_checked_hour = now->tm_hour;
    s_alarm.last_checked_minute = now->tm_min;

    // Don't check if alarm already triggered
    if (s_alarm.triggered) {
        // Check for auto-dismiss
        time_t elapsed = time(NULL) - s_alarm.triggered_time;
        if (elapsed > (ALARM_AUTO_DISMISS_MINUTES * 60)) {
            ESP_LOGI(TAG, "Alarm %d auto-dismissed after timeout", s_alarm.triggered_index);
            uint8_t index = s_alarm.triggered_index;
            audio_player_stop();
            s_alarm.triggered = false;
            s_alarm.triggered_index = 0xFF;

            if (s_alarm.callback) {
                s_alarm.callback(ALARM_EVENT_EXPIRED, index, s_alarm.callback_user_data);
            }
        }
        return;
    }

    // Check each enabled alarm
    uint8_t wday_mask = (1 << now->tm_wday);  // tm_wday: 0=Sunday

    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!s_alarm.alarms[i].enabled) {
            continue;
        }

        // Check time match
        if (s_alarm.alarms[i].hour != now->tm_hour ||
            s_alarm.alarms[i].minute != now->tm_min) {
            continue;
        }

        // Check day match (0 = one-time, always triggers)
        if (s_alarm.alarms[i].repeat_days != ALARM_REPEAT_ONCE &&
            !(s_alarm.alarms[i].repeat_days & wday_mask)) {
            continue;
        }

        // Trigger alarm
        trigger_alarm(i);
        return;  // Only trigger one alarm at a time
    }

check_snooze:
    // Check snooze
    if (s_alarm.snooze_active && !s_alarm.triggered) {
        if (time(NULL) >= s_alarm.snooze_until) {
            ESP_LOGI(TAG, "Snooze alarm %d re-triggering", s_alarm.snooze_index);
            s_alarm.snooze_active = false;
            trigger_alarm(s_alarm.snooze_index);
        }
    }
}

esp_err_t alarm_register_callback(alarm_event_cb_t callback, void *user_data)
{
    s_alarm.callback = callback;
    s_alarm.callback_user_data = user_data;
    return ESP_OK;
}

esp_err_t alarm_get_all(alarm_entry_t alarms[ALARM_MAX_COUNT])
{
    if (!alarms) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_alarm.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    memcpy(alarms, s_alarm.alarms, sizeof(alarm_entry_t) * ALARM_MAX_COUNT);

    xSemaphoreGive(s_alarm.mutex);
    return ESP_OK;
}
