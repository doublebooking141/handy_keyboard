/**
 * @file flick_input.c
 * @brief Japanese Flick Input to Romaji Conversion
 *
 * Flick direction mapping:
 *   - Center (tap): あ段 (a)
 *   - Left: い段 (i)
 *   - Up: う段 (u)
 *   - Right: え段 (e)
 *   - Down: お段 (o)
 */

#include "flick_input.h"
#include "lvgl.h"

// External UI panel references
extern lv_obj_t *ui_Panel3;   // あ
extern lv_obj_t *ui_Panel4;   // か
extern lv_obj_t *ui_Panel5;   // さ
extern lv_obj_t *ui_Panel8;   // た
extern lv_obj_t *ui_Panel9;   // な
extern lv_obj_t *ui_Panel10;  // は
extern lv_obj_t *ui_Panel13;  // ま
extern lv_obj_t *ui_Panel14;  // や
extern lv_obj_t *ui_Panel15;  // ら
extern lv_obj_t *ui_Panel19;  // わ

// ============================================================================
// Flick Mapping Table
// ============================================================================

// Each row: [center, left(i), up(u), right(e), down(o)]
static const char *kana_romaji[KANA_COUNT][5] = {
    // あ行
    {"a",  "i",  "u",  "e",  "o"},
    // か行
    {"ka", "ki", "ku", "ke", "ko"},
    // さ行
    {"sa", "si", "su", "se", "so"},
    // た行
    {"ta", "ti", "tu", "te", "to"},
    // な行
    {"na", "ni", "nu", "ne", "no"},
    // は行
    {"ha", "hi", "hu", "he", "ho"},
    // ま行
    {"ma", "mi", "mu", "me", "mo"},
    // や行
    {"ya", "",   "yu", "",   "yo"},  // yi, ye are empty (rarely used)
    // ら行
    {"ra", "ri", "ru", "re", "ro"},
    // わ行
    {"wa", "wo", "nn", "",   "n"},   // Special: nn = ん, n = ん (single)
};

// ============================================================================
// Direction Conversion
// ============================================================================

flick_direction_t lv_dir_to_flick(uint8_t lv_dir)
{
    switch (lv_dir) {
    case LV_DIR_LEFT:
        return FLICK_LEFT;
    case LV_DIR_TOP:
        return FLICK_UP;
    case LV_DIR_RIGHT:
        return FLICK_RIGHT;
    case LV_DIR_BOTTOM:
        return FLICK_DOWN;
    default:
        return FLICK_CENTER;
    }
}

flick_direction_t angle_to_flick(int16_t angle_deg)
{
    // Normalize angle to 0-360
    while (angle_deg < 0) angle_deg += 360;
    angle_deg = angle_deg % 360;

    // Map angle to direction
    // Up: 315-360, 0-45
    // Right: 45-135
    // Down: 135-225
    // Left: 225-315
    if (angle_deg >= 315 || angle_deg < 45) {
        return FLICK_UP;
    } else if (angle_deg < 135) {
        return FLICK_RIGHT;
    } else if (angle_deg < 225) {
        return FLICK_DOWN;
    } else {
        return FLICK_LEFT;
    }
}

// ============================================================================
// Romaji Lookup
// ============================================================================

const char *flick_to_romaji(kana_row_t row, flick_direction_t dir)
{
    if (row >= KANA_COUNT || dir > FLICK_DOWN) {
        return NULL;
    }

    return kana_romaji[row][dir];
}

// ============================================================================
// Panel to Row Mapping
// ============================================================================

int flick_panel_to_row(void *panel)
{
    if (panel == ui_Panel3)  return KANA_A;
    if (panel == ui_Panel4)  return KANA_KA;
    if (panel == ui_Panel5)  return KANA_SA;
    if (panel == ui_Panel8)  return KANA_TA;
    if (panel == ui_Panel9)  return KANA_NA;
    if (panel == ui_Panel10) return KANA_HA;
    if (panel == ui_Panel13) return KANA_MA;
    if (panel == ui_Panel14) return KANA_YA;
    if (panel == ui_Panel15) return KANA_RA;
    if (panel == ui_Panel19) return KANA_WA;
    return -1;  // Not a kana panel
}
