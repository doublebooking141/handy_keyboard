/**
 * @file alarm.h
 * @brief Alarm Management API
 *
 * Provides multiple alarm support with NVS persistence,
 * time checking, and audio playback triggers.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of alarms */
#define ALARM_MAX_COUNT      8

/** Maximum length of sound file path */
#define ALARM_SOUND_PATH_LEN 64

/** Default alarm sound path (when sound_path is empty) */
#define ALARM_DEFAULT_SOUND  "/sdcard/sounds/alarm.wav"

/** Snooze duration in minutes */
#define ALARM_SNOOZE_MINUTES 5

/**
 * @brief Repeat day flags (bitmask)
 */
typedef enum {
    ALARM_REPEAT_SUN = (1 << 0),  /**< Sunday */
    ALARM_REPEAT_MON = (1 << 1),  /**< Monday */
    ALARM_REPEAT_TUE = (1 << 2),  /**< Tuesday */
    ALARM_REPEAT_WED = (1 << 3),  /**< Wednesday */
    ALARM_REPEAT_THU = (1 << 4),  /**< Thursday */
    ALARM_REPEAT_FRI = (1 << 5),  /**< Friday */
    ALARM_REPEAT_SAT = (1 << 6),  /**< Saturday */

    ALARM_REPEAT_WEEKDAYS = (ALARM_REPEAT_MON | ALARM_REPEAT_TUE |
                             ALARM_REPEAT_WED | ALARM_REPEAT_THU | ALARM_REPEAT_FRI),
    ALARM_REPEAT_WEEKEND  = (ALARM_REPEAT_SAT | ALARM_REPEAT_SUN),
    ALARM_REPEAT_EVERYDAY = 0x7F,
    ALARM_REPEAT_ONCE     = 0x00,  /**< One-time alarm (no repeat) */
} alarm_repeat_day_t;

/**
 * @brief Alarm entry structure
 */
typedef struct {
    uint8_t  hour;                            /**< Hour (0-23) */
    uint8_t  minute;                          /**< Minute (0-59) */
    uint8_t  repeat_days;                     /**< Bitmask of alarm_repeat_day_t */
    bool     enabled;                         /**< Alarm enabled flag */
    char     sound_path[ALARM_SOUND_PATH_LEN]; /**< Sound file path (empty = default) */
} alarm_entry_t;

/**
 * @brief Alarm event types
 */
typedef enum {
    ALARM_EVENT_TRIGGERED,   /**< Alarm triggered - sound starting */
    ALARM_EVENT_DISMISSED,   /**< Alarm dismissed by user */
    ALARM_EVENT_SNOOZED,     /**< Alarm snoozed */
    ALARM_EVENT_EXPIRED,     /**< Alarm expired (auto-dismiss after timeout) */
} alarm_event_t;

/**
 * @brief Alarm event callback
 *
 * @param event Event type
 * @param index Alarm index that triggered the event
 * @param user_data User-provided context
 */
typedef void (*alarm_event_cb_t)(alarm_event_t event, uint8_t index, void *user_data);

/**
 * @brief Initialize alarm system
 *
 * Loads alarms from NVS and starts time checking task.
 *
 * @return ESP_OK on success
 */
esp_err_t alarm_init(void);

/**
 * @brief Deinitialize alarm system
 */
void alarm_deinit(void);

/**
 * @brief Get alarm entry
 *
 * @param index Alarm index (0 to ALARM_MAX_COUNT-1)
 * @param alarm Output alarm entry
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if index out of range
 */
esp_err_t alarm_get(uint8_t index, alarm_entry_t *alarm);

/**
 * @brief Set alarm entry
 *
 * Saves to NVS immediately.
 *
 * @param index Alarm index (0 to ALARM_MAX_COUNT-1)
 * @param alarm Alarm entry to set
 * @return ESP_OK on success
 */
esp_err_t alarm_set(uint8_t index, const alarm_entry_t *alarm);

/**
 * @brief Delete (disable and clear) alarm entry
 *
 * @param index Alarm index (0 to ALARM_MAX_COUNT-1)
 * @return ESP_OK on success
 */
esp_err_t alarm_delete(uint8_t index);

/**
 * @brief Get count of enabled alarms
 *
 * @return Number of enabled alarms
 */
uint8_t alarm_get_enabled_count(void);

/**
 * @brief Check if any alarm is currently triggered
 *
 * @return true if alarm is playing
 */
bool alarm_is_triggered(void);

/**
 * @brief Get index of currently triggered alarm
 *
 * @return Triggered alarm index, or 0xFF if none
 */
uint8_t alarm_get_triggered_index(void);

/**
 * @brief Dismiss currently triggered alarm
 *
 * Stops alarm sound and clears triggered state.
 *
 * @return ESP_OK on success
 */
esp_err_t alarm_dismiss(void);

/**
 * @brief Snooze currently triggered alarm
 *
 * Stops alarm sound and schedules re-trigger after ALARM_SNOOZE_MINUTES.
 *
 * @return ESP_OK on success
 */
esp_err_t alarm_snooze(void);

/**
 * @brief Check time and trigger alarms if needed
 *
 * Call this periodically (e.g., every second from RTC update task).
 *
 * @param now Current time
 */
void alarm_check_time(const struct tm *now);

/**
 * @brief Register alarm event callback
 *
 * @param callback Callback function (NULL to unregister)
 * @param user_data User context passed to callback
 * @return ESP_OK on success
 */
esp_err_t alarm_register_callback(alarm_event_cb_t callback, void *user_data);

/**
 * @brief Get all alarms (for UI display)
 *
 * @param alarms Array of ALARM_MAX_COUNT entries
 * @return ESP_OK on success
 */
esp_err_t alarm_get_all(alarm_entry_t alarms[ALARM_MAX_COUNT]);

#ifdef __cplusplus
}
#endif
