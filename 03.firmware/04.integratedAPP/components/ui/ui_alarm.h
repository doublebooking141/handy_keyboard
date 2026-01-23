/**
 * @file ui_alarm.h
 * @brief Alarm Settings UI API
 *
 * Provides alarm configuration screen and trigger popup.
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize alarm UI
 *
 * Creates alarm settings UI in the settings panel.
 *
 * @param parent Parent container (settings panel)
 */
void ui_alarm_init(lv_obj_t *parent);

/**
 * @brief Deinitialize alarm UI
 *
 * Cleans up UI elements.
 */
void ui_alarm_deinit(void);

/**
 * @brief Update alarm list display
 *
 * Refreshes the alarm list with current alarm data.
 */
void ui_alarm_update_list(void);

/**
 * @brief Show alarm trigger popup
 *
 * Displays full-screen alarm popup with dismiss/snooze buttons.
 *
 * @param alarm_index Index of triggered alarm
 */
void ui_alarm_show_trigger_popup(uint8_t alarm_index);

/**
 * @brief Hide alarm trigger popup
 */
void ui_alarm_hide_trigger_popup(void);

/**
 * @brief Check if trigger popup is visible
 *
 * @return true if popup is visible
 */
bool ui_alarm_is_popup_visible(void);

#ifdef __cplusplus
}
#endif
