/**
 * @file flick_input.h
 * @brief Japanese Flick Input Handler
 *
 * Converts flick gestures on kana panels to romaji strings.
 * Flick directions:
 *   - Center (tap): あ段 (a)
 *   - Left: い段 (i)
 *   - Up: う段 (u)
 *   - Right: え段 (e)
 *   - Down: お段 (o)
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flick direction enumeration
 */
typedef enum {
    FLICK_CENTER = 0,   // Tap (no flick)
    FLICK_LEFT,         // い段
    FLICK_UP,           // う段
    FLICK_RIGHT,        // え段
    FLICK_DOWN,         // お段
} flick_direction_t;

/**
 * @brief Kana panel indices
 */
typedef enum {
    KANA_A = 0,     // あ行
    KANA_KA,        // か行
    KANA_SA,        // さ行
    KANA_TA,        // た行
    KANA_NA,        // な行
    KANA_HA,        // は行
    KANA_MA,        // ま行
    KANA_YA,        // や行
    KANA_RA,        // ら行
    KANA_WA,        // わ行
    KANA_COUNT
} kana_row_t;

/**
 * @brief Convert LVGL gesture direction to flick direction
 *
 * @param lv_dir LVGL direction (LV_DIR_TOP, LV_DIR_BOTTOM, etc.)
 * @return Corresponding flick direction
 */
flick_direction_t lv_dir_to_flick(uint8_t lv_dir);

/**
 * @brief Convert angle to flick direction
 *
 * @param angle_deg Angle in degrees (0=up, 90=right, 180=down, 270=left)
 * @return Corresponding flick direction
 */
flick_direction_t angle_to_flick(int16_t angle_deg);

/**
 * @brief Get romaji string for kana panel + flick direction
 *
 * @param row Kana row (あ/か/さ/...)
 * @param dir Flick direction
 * @return Romaji string (e.g., "ka", "ki", "ku")
 */
const char *flick_to_romaji(kana_row_t row, flick_direction_t dir);

/**
 * @brief Get kana row from panel pointer
 *
 * @param panel LVGL panel object pointer
 * @return Kana row index, or -1 if not a kana panel
 */
int flick_panel_to_row(void *panel);

#ifdef __cplusplus
}
#endif
