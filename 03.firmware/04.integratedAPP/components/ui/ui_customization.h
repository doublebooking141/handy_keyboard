/**
 * @file ui_customization.h
 * @brief UI customization and styling for Handy Keyboard
 *
 * This file handles dynamic UI modifications:
 * - Label text updates
 * - Style applications (colors, fonts)
 * - Header synchronization
 * - Analog clock hands drawing
 */

#ifndef _UI_CUSTOMIZATION_H
#define _UI_CUSTOMIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Initialize UI customization
 *
 * Call this after ui_navigation_init() to apply custom styles and modifications
 */
void ui_customization_init(void);

/**
 * @brief Update clock hands based on current time
 *
 * @param hour Current hour (0-23)
 * @param minute Current minute (0-59)
 * @param second Current second (0-59)
 */
void ui_clock_update_hands(int hour, int minute, int second);

/**
 * @brief Update header with current date/time
 *
 * @param date_str Date string (e.g., "2025/01/12 (Sun)")
 * @param time_str Time string (e.g., "15:30")
 * @param battery_pct Battery percentage (0-100)
 */
void ui_header_update(const char *date_str, const char *time_str, int battery_pct);

#ifdef __cplusplus
}
#endif

#endif /* _UI_CUSTOMIZATION_H */
