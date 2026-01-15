/**
 * @file ui_navigation.h
 * @brief Screen navigation for Handy Keyboard UI
 */

#ifndef _UI_NAVIGATION_H
#define _UI_NAVIGATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize navigation event handlers
 *
 * Call this after ui_init() to add screen navigation functionality
 */
void ui_navigation_init(void);

/**
 * @brief Update navigation handlers for newly created screens
 *
 * This is called automatically by a timer, but can be called manually
 * after screen transitions if immediate navigation setup is needed.
 */
void ui_navigation_update(void);

#ifdef __cplusplus
}
#endif

#endif /* _UI_NAVIGATION_H */
