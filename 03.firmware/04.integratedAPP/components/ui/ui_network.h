/**
 * @file ui_network.h
 * @brief Network Settings UI - WiFi, NTP, and WOL controls
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize network settings UI
 *
 * Creates the Network & Time settings section in the parent container.
 *
 * @param[in] parent Parent LVGL object to add UI to
 */
void ui_network_init(lv_obj_t *parent);

/**
 * @brief Deinitialize network settings UI
 */
void ui_network_deinit(void);

/**
 * @brief Update network UI state
 *
 * Call periodically to refresh WiFi status, sync status, etc.
 */
void ui_network_update(void);

#ifdef __cplusplus
}
#endif
