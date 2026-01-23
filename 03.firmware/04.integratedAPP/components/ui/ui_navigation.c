/**
 * @file ui_navigation.c
 * @brief Screen navigation event handlers for Handy Keyboard UI
 *
 * Navigation Flow:
 * - Menu → KeyBoards → JPKeyboardScreen
 * - JPKeyboardScreen: Panel2(絵)→Menu, Panel17(あ/a)→AtoZ, Panel12(<+>)→Cursor
 * - AtoZKeyboardScreen: Header→JP, custom keyboard button→JP
 * - CursorScreen: Panel1(絵)→Menu, Panel31([a])→AtoZ, Panel36(あ)→JP
 * - Clock → Menu (via Image11)
 * - DateAndTime → Menu (via Back label)
 * - Settings → Menu (via back image button)
 */

#include "ui.h"
#include "ui_helpers.h"
#include "esp_log.h"
#include <string.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BLE HID
#include "ble_hid.h"
#include "ble_hid_keycodes.h"
#include "flick_input.h"

// BSP Touch for multi-touch support
#include "bsp/touch.h"
#include "esp_lcd_touch.h"

// Alarm UI
#include "ui_alarm.h"
#include "alarm.h"

// Background settings
#include "sdcard.h"
#include "ui_background.h"

static const char *TAG = "UI_NAV";

// ============================================================================
// Touchpad Configuration
// ============================================================================

#define TOUCHPAD_TAP_THRESHOLD      12      // Max movement for tap (pixels)
#define TOUCHPAD_TAP_TIME_MS        300     // Max time for tap (ms)
#define TOUCHPAD_SCROLL_THRESHOLD   8       // Min accumulated movement for scroll start
#define TOUCHPAD_PIXELS_PER_SCROLL  15      // Pixels of movement per scroll tick
#define TOUCHPAD_SENSITIVITY        2       // Mouse movement multiplier
#define TOUCHPAD_STRENGTH_2FINGER   120     // Strength threshold to detect close 2-fingers

// Touchpad operating mode
typedef enum {
    TOUCHPAD_MODE_MOUSE,    // Switch OFF: mouse movement + clicks
    TOUCHPAD_MODE_SCROLL,   // Switch ON: scroll operations
} touchpad_mode_t;

// Touchpad gesture state
typedef struct {
    touchpad_mode_t mode;
    lv_point_t start_pos;           // Touch start position
    lv_point_t last_pos;            // Last touch position (for cursor)
    lv_point_t scroll_anchor;       // Anchor for scroll calculation
    int32_t scroll_accum_x;         // Accumulated X movement for scroll
    int32_t scroll_accum_y;         // Accumulated Y movement for scroll
    uint8_t initial_finger_count;   // Finger count at touch start
    uint32_t touch_start_time;      // Touch start timestamp (ms)
    bool is_active;                 // Currently tracking a touch
} touchpad_state_t;

static touchpad_state_t g_touchpad_state = {
    .mode = TOUCHPAD_MODE_MOUSE,
    .is_active = false
};

// Forward declarations for screen init functions
extern void ui_JPKeyboardScreen_screen_init(void);
extern void ui_AtoZKeyboardScreen_screen_init(void);
extern void ui_AnalogClockWithBackgroud_screen_init(void);
extern void ui_SettingScreen_screen_init(void);
extern void ui_CursorScreen_screen_init(void);
extern void ui_DateAndTime_screen_init(void);
extern void ui_MenuScreen_screen_init(void);

// Screen objects
extern lv_obj_t *ui_MenuScreen;
extern lv_obj_t *ui_JPKeyboardScreen;
extern lv_obj_t *ui_AtoZKeyboardScreen;
extern lv_obj_t *ui_AnalogClockWithBackgroud;
extern lv_obj_t *ui_SettingScreen;
extern lv_obj_t *ui_CursorScreen;
extern lv_obj_t *ui_DateAndTime;

// Menu screen objects
extern lv_obj_t *ui_KeyBoards;
extern lv_obj_t *ui_Clock;
extern lv_obj_t *ui_PowerOnPC;
extern lv_obj_t *ui_LightOn;
extern lv_obj_t *ui_Setthing;

// Header panels
extern lv_obj_t *ui_HeaderPanel;    // JPKeyboardScreen
extern lv_obj_t *ui_HeaderPanel1;   // AtoZKeyboardScreen
extern lv_obj_t *ui_HeaderPanel2;   // CursorScreen
extern lv_obj_t *ui_HeaderPanel3;   // SettingScreen

// JPKeyboardScreen buttons
extern lv_obj_t *ui_Panel2;         // Container for Image2
extern lv_obj_t *ui_Image2;         // Top-left (絵) -> Menu (actual clickable image)
extern lv_obj_t *ui_Panel12;        // <+> -> Cursor
extern lv_obj_t *ui_Panel17;        // あ/a -> AtoZ

// JPKeyboardScreen kana panels
extern lv_obj_t *ui_Panel3;         // あ
extern lv_obj_t *ui_Panel4;         // か
extern lv_obj_t *ui_Panel5;         // さ
extern lv_obj_t *ui_Panel8;         // た
extern lv_obj_t *ui_Panel9;         // な
extern lv_obj_t *ui_Panel10;        // は
extern lv_obj_t *ui_Panel13;        // ま
extern lv_obj_t *ui_Panel14;        // や
extern lv_obj_t *ui_Panel15;        // ら
extern lv_obj_t *ui_Panel19;        // わ

// JPKeyboardScreen special keys (panels and their clickable children)
extern lv_obj_t *ui_Panel6;         // Backspace panel
extern lv_obj_t *ui_Image3;         // Backspace image (inside Panel6)
extern lv_obj_t *ui_Panel7;         // ← Left arrow panel
extern lv_obj_t *ui_Image4;         // Left arrow image (inside Panel7)
extern lv_obj_t *ui_Panel11;        // → Right arrow panel
extern lv_obj_t *ui_Image1;         // Right arrow image (inside Panel11)
extern lv_obj_t *ui_Panel16;        // Space panel
extern lv_obj_t *ui_Label11;        // Space label (inside Panel16)
extern lv_obj_t *ui_Panel18;        // Modifier (小さくなる/濁点/半濁点)
extern lv_obj_t *ui_Label13;        // Modifier label (inside Panel18)
extern lv_obj_t *ui_Panel20;        // 記号 (、？。…！)
extern lv_obj_t *ui_Label15;        // Symbol label (inside Panel20)
extern lv_obj_t *ui_Panel21;        // Enter panel
extern lv_obj_t *ui_Image5;         // Enter image (inside Panel21)

// AtoZKeyboardScreen
extern lv_obj_t *ui_OtherKeyboard;  // lv_keyboard widget

// TouchAndScrollPanel (touchpad area on each keyboard screen)
extern lv_obj_t *ui_TouchAndScrollPanel;    // JPKeyboardScreen
extern lv_obj_t *ui_TouchAndScrollPanel1;   // AtoZKeyboardScreen
extern lv_obj_t *ui_TouchAndScrollPanel2;   // CursorScreen

// Mode toggle switches (inside TouchAndScrollPanel)
extern lv_obj_t *ui_Switch1;    // JPKeyboardScreen
extern lv_obj_t *ui_Switch2;    // AtoZKeyboardScreen
extern lv_obj_t *ui_Switch3;    // CursorScreen

// CursorScreen buttons (panels and their clickable children)
extern lv_obj_t *ui_Panel1;         // Container for Image6
extern lv_obj_t *ui_Image6;         // Top-left (絵) -> Menu (actual clickable image)
extern lv_obj_t *ui_Panel31;        // [a] -> AtoZ (container)
extern lv_obj_t *ui_Label22;        // ?123 label (inside Panel31, for AtoZ nav)
extern lv_obj_t *ui_Panel36;        // あ -> JP Keyboard (container)
extern lv_obj_t *ui_Label27;        // あ/a label (inside Panel36, for JP nav)

// CursorScreen key elements (images/labels inside panels)
extern lv_obj_t *ui_Label16;        // ESC label (inside Panel22)
extern lv_obj_t *ui_Label18;        // Enter label (inside Panel24)
extern lv_obj_t *ui_Image14;        // Up arrow image (inside Panel37)
extern lv_obj_t *ui_Image13;        // Left arrow image (inside Panel38)
extern lv_obj_t *ui_Image15;        // Down arrow image (inside Panel39)
extern lv_obj_t *ui_Image12;        // Right arrow image (inside Panel33)
extern lv_obj_t *ui_Label17;        // Paste label (inside Panel23)
extern lv_obj_t *ui_Label20;        // Copy label (inside Panel28)
extern lv_obj_t *ui_Label25;        // Undo label (inside Panel34)
extern lv_obj_t *ui_Label21;        // Redo label (inside Panel29)
extern lv_obj_t *ui_Label26;        // Backspace label (inside Panel35)

// AnalogClockWithBackgroud back button
extern lv_obj_t *ui_Image11;

// DateAndTime back label
extern lv_obj_t *ui_Back;

// SettingScreen elements
extern lv_obj_t *ui_Panel43;

// Back button image declaration
LV_IMG_DECLARE(ui_img_2026807732);  // Uターン矢印 2.png

// Track which screens have been initialized with navigation
// Also track the screen object to detect re-creation
static lv_obj_t *last_jp_screen = NULL;
static lv_obj_t *last_atoz_screen = NULL;
static lv_obj_t *last_cursor_screen = NULL;
static lv_obj_t *last_clock_screen = NULL;
static lv_obj_t *last_datetime_screen = NULL;
static lv_obj_t *last_settings_screen = NULL;

// Dynamic elements
static lv_obj_t *setting_back_btn = NULL;

// BLE Settings UI elements
static lv_obj_t *ble_status_label = NULL;
static lv_obj_t *ble_dropdown = NULL;
static lv_obj_t *ble_connect_btn = NULL;
static lv_obj_t *ble_connect_label = NULL;
static lv_obj_t *ble_delete_btn = NULL;
static lv_obj_t *ble_clear_btn = NULL;

// Bonded device storage for dropdown selection
#define MAX_BONDED_DEVICES 5
static uint8_t g_bonded_addrs[MAX_BONDED_DEVICES][6];
static int g_bonded_count = 0;

// BLE state for UI updates (volatile for cross-task visibility)
static volatile ble_hid_state_t g_ble_ui_state = BLE_HID_STATE_IDLE;
static volatile bool g_ble_state_changed = false;

// Alarm state for UI updates (volatile for cross-task visibility)
static volatile bool g_alarm_triggered = false;
static volatile uint8_t g_alarm_triggered_index = 0xFF;

// Background Settings UI elements
static lv_obj_t *bg_touchpad_dropdown = NULL;
static lv_obj_t *bg_clock_dropdown = NULL;
static bool bg_ui_created = false;

// Background file list storage
#define MAX_BG_FILES 32
static sdcard_file_t s_bg_files[MAX_BG_FILES];
static size_t s_bg_file_count = 0;

// ============================================================================
// Navigation Callbacks
// ============================================================================

static void nav_to_menu(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_MenuScreen_screen_init);
    }
}

static void nav_to_jp_keyboard(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_JPKeyboardScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_JPKeyboardScreen_screen_init);
    }
}

static void nav_to_atoz_keyboard(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_AtoZKeyboardScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, ui_AtoZKeyboardScreen_screen_init);
    }
}

static void nav_to_cursor(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_CursorScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, ui_CursorScreen_screen_init);
    }
}

static void nav_to_clock(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_AnalogClockWithBackgroud, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_AnalogClockWithBackgroud_screen_init);
    }
}

static void nav_to_settings(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        _ui_screen_change(&ui_SettingScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_SettingScreen_screen_init);
    }
}

static void nav_power_on_pc(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        // WoL - not implemented yet
    }
}

static void nav_light_on(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        // Light On - not implemented yet
    }
}

// ============================================================================
// Touchpad Event Handlers
// ============================================================================

/**
 * @brief Read current finger count from touch controller
 *
 * Performs a thread-safe I2C read to get the current multi-touch state.
 * Also detects close 2-finger touches via strength analysis.
 */
static uint8_t touchpad_read_finger_count(void)
{
    esp_lcd_touch_handle_t handle = bsp_touch_get_handle();
    if (!handle) {
        return 1;  // Fallback to single finger
    }

    // Read fresh touch data with thread-safe access
    uint16_t x[5], y[5], strength[5];
    uint8_t point_num = 0;

    esp_err_t ret = bsp_touch_read_data_safe(
        handle, x, y, strength, &point_num, 5, pdMS_TO_TICKS(10)
    );

    if (ret == ESP_OK && point_num > 0) {
        // Check for close 2-fingers: single point with high strength
        if (point_num == 1 && strength[0] >= TOUCHPAD_STRENGTH_2FINGER) {
            ESP_LOGW(TAG, "Touch: 1 point, strength=%d -> treating as 2 fingers", strength[0]);
            return 2;
        }
        ESP_LOGD(TAG, "Touch read: %d fingers, strength[0]=%d", point_num, strength[0]);
        return point_num;
    }
    return 1;
}

/**
 * @brief Handle touchpad press start
 */
static void touchpad_pressed_cb(lv_event_t *e)
{
    lv_point_t point;
    lv_indev_t *indev = lv_indev_active();
    lv_indev_get_point(indev, &point);

    g_touchpad_state.start_pos = point;
    g_touchpad_state.last_pos = point;
    g_touchpad_state.scroll_anchor = point;
    g_touchpad_state.scroll_accum_x = 0;
    g_touchpad_state.scroll_accum_y = 0;
    g_touchpad_state.initial_finger_count = touchpad_read_finger_count();
    g_touchpad_state.touch_start_time = lv_tick_get();
    g_touchpad_state.is_active = true;

    ESP_LOGW(TAG, "Touchpad pressed: fingers=%d, pos=(%ld,%ld)",
             g_touchpad_state.initial_finger_count, point.x, point.y);
}

/**
 * @brief Send proportional scroll based on accumulated movement
 *
 * Calculates scroll ticks based on total movement from anchor point.
 * Updates anchor when scroll is sent to allow continuous scrolling.
 */
static void touchpad_send_proportional_scroll(lv_point_t *point)
{
    // Calculate total movement from scroll anchor
    int32_t total_dx = point->x - g_touchpad_state.scroll_anchor.x;
    int32_t total_dy = point->y - g_touchpad_state.scroll_anchor.y;
    int32_t abs_dx = (total_dx < 0) ? -total_dx : total_dx;
    int32_t abs_dy = (total_dy < 0) ? -total_dy : total_dy;

    // Only start scrolling after threshold
    if (abs_dx < TOUCHPAD_SCROLL_THRESHOLD && abs_dy < TOUCHPAD_SCROLL_THRESHOLD) {
        return;
    }

    // Determine primary scroll direction and calculate ticks
    int8_t wheel = 0, h_wheel = 0;

    if (abs_dy >= abs_dx) {
        // Vertical scroll
        int32_t scroll_ticks = total_dy / TOUCHPAD_PIXELS_PER_SCROLL;
        if (scroll_ticks != 0) {
            // Traditional scroll direction (swipe down = scroll down)
            wheel = (int8_t)(scroll_ticks > 127 ? 127 : (scroll_ticks < -127 ? -127 : scroll_ticks));
            // Update anchor by consumed pixels
            g_touchpad_state.scroll_anchor.y += scroll_ticks * TOUCHPAD_PIXELS_PER_SCROLL;
        }
    } else {
        // Horizontal scroll
        int32_t scroll_ticks = total_dx / TOUCHPAD_PIXELS_PER_SCROLL;
        if (scroll_ticks != 0) {
            // Inverted horizontal (swipe right = scroll left)
            h_wheel = (int8_t)(scroll_ticks > 127 ? 127 : (scroll_ticks < -127 ? -127 : -scroll_ticks));
            // Update anchor by consumed pixels
            g_touchpad_state.scroll_anchor.x += scroll_ticks * TOUCHPAD_PIXELS_PER_SCROLL;
        }
    }

    if (wheel != 0 || h_wheel != 0) {
        ble_hid_send_mouse(0, 0, 0, wheel, h_wheel);
    }
}

/**
 * @brief Handle touchpad movement (pressing)
 */
static void touchpad_pressing_cb(lv_event_t *e)
{
    if (!g_touchpad_state.is_active) return;

    lv_point_t point;
    lv_indev_t *indev = lv_indev_active();
    lv_indev_get_point(indev, &point);

    int32_t dx = point.x - g_touchpad_state.last_pos.x;
    int32_t dy = point.y - g_touchpad_state.last_pos.y;

    // Use initial finger count (captured at touch start)
    uint8_t fingers = g_touchpad_state.initial_finger_count;

    if (g_touchpad_state.mode == TOUCHPAD_MODE_MOUSE) {
        // Mouse mode: single finger = cursor movement
        if (fingers == 1 && (dx != 0 || dy != 0)) {
            // Scale movement for sensitivity
            int32_t scaled_dx = dx * TOUCHPAD_SENSITIVITY;
            int32_t scaled_dy = dy * TOUCHPAD_SENSITIVITY;

            // Clamp to valid range
            if (scaled_dx > 127) scaled_dx = 127;
            if (scaled_dx < -127) scaled_dx = -127;
            if (scaled_dy > 127) scaled_dy = 127;
            if (scaled_dy < -127) scaled_dy = -127;

            if (scaled_dx != 0 || scaled_dy != 0) {
                ble_hid_send_mouse(0, (int8_t)scaled_dx, (int8_t)scaled_dy, 0, 0);
            }
        }
        // 2-finger movement: proportional scroll
        else if (fingers >= 2) {
            touchpad_send_proportional_scroll(&point);
        }
    } else {
        // Scroll mode: any movement = proportional scroll
        touchpad_send_proportional_scroll(&point);
    }

    g_touchpad_state.last_pos = point;
}

/**
 * @brief Handle touchpad release
 */
static void touchpad_released_cb(lv_event_t *e)
{
    if (!g_touchpad_state.is_active) return;

    lv_point_t point;
    lv_indev_t *indev = lv_indev_active();
    lv_indev_get_point(indev, &point);

    // Calculate total movement
    int32_t total_dx = point.x - g_touchpad_state.start_pos.x;
    int32_t total_dy = point.y - g_touchpad_state.start_pos.y;
    int32_t abs_dx = (total_dx < 0) ? -total_dx : total_dx;
    int32_t abs_dy = (total_dy < 0) ? -total_dy : total_dy;

    // Calculate duration
    uint32_t duration = lv_tick_get() - g_touchpad_state.touch_start_time;

    // Check for tap gesture (small movement, short duration)
    bool is_tap = (abs_dx < TOUCHPAD_TAP_THRESHOLD &&
                   abs_dy < TOUCHPAD_TAP_THRESHOLD &&
                   duration < TOUCHPAD_TAP_TIME_MS);

    ESP_LOGW(TAG, "Touchpad released: fingers=%d, movement=(%ld,%ld), duration=%lums, is_tap=%d",
             g_touchpad_state.initial_finger_count, total_dx, total_dy, (unsigned long)duration, is_tap);

    if (is_tap) {
        uint8_t button = 0;

        if (g_touchpad_state.mode == TOUCHPAD_MODE_MOUSE) {
            // Mouse mode tap gestures
            switch (g_touchpad_state.initial_finger_count) {
                case 1:
                    button = MOUSE_BTN_LEFT;
                    ESP_LOGW(TAG, "Tap gesture: Left click (1 finger)");
                    break;
                case 2:
                    button = MOUSE_BTN_RIGHT;
                    ESP_LOGW(TAG, "Tap gesture: Right click (2 fingers)");
                    break;
                case 3:
                default:
                    button = MOUSE_BTN_MIDDLE;
                    ESP_LOGW(TAG, "Tap gesture: Middle click (%d fingers)",
                             g_touchpad_state.initial_finger_count);
                    break;
            }
        } else {
            // Scroll mode: tap = middle click
            button = MOUSE_BTN_MIDDLE;
            ESP_LOGW(TAG, "Tap gesture (scroll mode): Middle click");
        }

        // Send click (press then release)
        ble_hid_send_mouse(button, 0, 0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        ble_hid_send_mouse(0, 0, 0, 0, 0);
    }

    g_touchpad_state.is_active = false;
}

/**
 * @brief Handle mode switch toggle
 */
static void touchpad_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);

    g_touchpad_state.mode = checked ? TOUCHPAD_MODE_SCROLL : TOUCHPAD_MODE_MOUSE;
    ESP_LOGI(TAG, "Touchpad mode: %s", checked ? "SCROLL" : "MOUSE");

    // Sync all switches to same state
    if (ui_Switch1 && ui_Switch1 != sw) {
        if (checked) lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
        else lv_obj_remove_state(ui_Switch1, LV_STATE_CHECKED);
    }
    if (ui_Switch2 && ui_Switch2 != sw) {
        if (checked) lv_obj_add_state(ui_Switch2, LV_STATE_CHECKED);
        else lv_obj_remove_state(ui_Switch2, LV_STATE_CHECKED);
    }
    if (ui_Switch3 && ui_Switch3 != sw) {
        if (checked) lv_obj_add_state(ui_Switch3, LV_STATE_CHECKED);
        else lv_obj_remove_state(ui_Switch3, LV_STATE_CHECKED);
    }
}

/**
 * @brief Register touchpad event handlers for a TouchAndScrollPanel
 */
static void setup_touchpad_panel(lv_obj_t *panel, lv_obj_t *sw)
{
    if (!panel) return;

    // Make panel clickable
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);

    // Register touch events
    lv_obj_add_event_cb(panel, touchpad_pressed_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(panel, touchpad_pressing_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(panel, touchpad_released_cb, LV_EVENT_RELEASED, NULL);

    // Register switch mode toggle
    if (sw) {
        lv_obj_add_event_cb(sw, touchpad_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    ESP_LOGD(TAG, "Touchpad panel setup complete");
}

// ============================================================================
// Keyboard Input Handlers
// ============================================================================

// Modifier key state (Panel18)
typedef enum {
    MODIFIER_NONE = 0,
    MODIFIER_SMALL,      // 小さくなる (xa, xya, xtu)
    MODIFIER_DAKUTEN,    // 濁点 (ka→ga, sa→za, ta→da, ha→ba)
    MODIFIER_HANDAKUTEN, // 半濁点 (ha→pa)
} modifier_state_t;

static modifier_state_t current_modifier = MODIFIER_NONE;

// Flick detection state
static lv_point_t flick_start_point = {0, 0};
static lv_obj_t *flick_active_panel = NULL;

#define FLICK_THRESHOLD 12  // Minimum pixels to detect flick

/**
 * @brief Calculate flick direction from start/end points
 */
static flick_direction_t calc_flick_direction(int32_t dx, int32_t dy)
{
    // Check if movement is large enough
    int32_t abs_dx = (dx < 0) ? -dx : dx;
    int32_t abs_dy = (dy < 0) ? -dy : dy;

    if (abs_dx < FLICK_THRESHOLD && abs_dy < FLICK_THRESHOLD) {
        return FLICK_CENTER;  // Tap, not flick
    }

    // Determine direction based on larger axis
    if (abs_dx > abs_dy) {
        // Horizontal movement
        return (dx < 0) ? FLICK_LEFT : FLICK_RIGHT;
    } else {
        // Vertical movement
        return (dy < 0) ? FLICK_UP : FLICK_DOWN;
    }
}

/**
 * @brief Apply modifier to romaji and send via BLE
 *
 * Modifiers:
 * - SMALL: a→xa, ya→xya, tu→xtu (あ行, や行, つ only)
 * - DAKUTEN: ka→ga, sa→za, ta→da, ha→ba (か/さ/た/は行 only)
 * - HANDAKUTEN: ha→pa (は行 only)
 *
 * Invalid combinations pass through unchanged.
 */
static void apply_modifier_and_send(const char *romaji, kana_row_t row)
{
    static char modified[8];  // Buffer for modified romaji
    modifier_state_t mod = current_modifier;
    current_modifier = MODIFIER_NONE;  // Reset after use

    if (mod == MODIFIER_NONE) {
        ble_hid_send_string(romaji);
        return;
    }

    if (mod == MODIFIER_SMALL) {
        // Small kana only valid for: あ行, や行, つ(tu), わ(wa)
        bool valid = false;
        if (row == KANA_A) {
            // あ行: a, i, u, e, o → xa, xi, xu, xe, xo
            valid = true;
        } else if (row == KANA_YA) {
            // や行: ya, yu, yo → xya, xyu, xyo (yi, ye are empty anyway)
            valid = (romaji[0] == 'y');
        } else if (row == KANA_TA && romaji[0] == 't' && romaji[1] == 'u') {
            // つ only: tu → xtu
            valid = true;
        } else if (row == KANA_WA && romaji[0] == 'w' && romaji[1] == 'a') {
            // わ only: wa → xwa
            valid = true;
        }

        if (valid) {
            modified[0] = 'x';
            strncpy(modified + 1, romaji, sizeof(modified) - 2);
            modified[sizeof(modified) - 1] = '\0';
            ble_hid_send_string(modified);
        } else {
            ble_hid_send_string(romaji);
        }
        return;
    }

    if (mod == MODIFIER_DAKUTEN) {
        // Dakuten valid for: か行(k), さ行(s), た行(t), は行(h), う(u→vu)
        bool valid = false;
        if (romaji[0] == 'k') {
            modified[0] = 'g';
            strncpy(modified + 1, romaji + 1, sizeof(modified) - 2);
            valid = true;
        } else if (romaji[0] == 's') {
            modified[0] = 'z';
            strncpy(modified + 1, romaji + 1, sizeof(modified) - 2);
            valid = true;
        } else if (romaji[0] == 't') {
            modified[0] = 'd';
            strncpy(modified + 1, romaji + 1, sizeof(modified) - 2);
            valid = true;
        } else if (romaji[0] == 'h') {
            modified[0] = 'b';
            strncpy(modified + 1, romaji + 1, sizeof(modified) - 2);
            valid = true;
        } else if (romaji[0] == 'u' && romaji[1] == '\0') {
            // Special case: u → vu (ゔ)
            modified[0] = 'v';
            modified[1] = 'u';
            modified[2] = '\0';
            valid = true;
        }

        if (valid) {
            modified[sizeof(modified) - 1] = '\0';
            ble_hid_send_string(modified);
        } else {
            ble_hid_send_string(romaji);
        }
        return;
    }

    if (mod == MODIFIER_HANDAKUTEN) {
        // Handakuten only valid for: は行(h)
        if (romaji[0] == 'h') {
            modified[0] = 'p';
            strncpy(modified + 1, romaji + 1, sizeof(modified) - 2);
            modified[sizeof(modified) - 1] = '\0';
            ble_hid_send_string(modified);
        } else {
            ble_hid_send_string(romaji);
        }
        return;
    }
}

/**
 * @brief Handle kana panel touch events for flick input
 */
static void kana_panel_input_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *panel = lv_event_get_target(e);

    // Get kana row from panel
    int row = flick_panel_to_row(panel);
    if (row < 0) return;

    if (code == LV_EVENT_PRESSED) {
        // Record start position
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &flick_start_point);
            flick_active_panel = panel;
            ESP_LOGD(TAG, "Touch start: (%ld, %ld)", (long)flick_start_point.x, (long)flick_start_point.y);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED && flick_active_panel == panel) {
        // Calculate flick direction
        lv_point_t end_point;
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &end_point);

            int32_t dx = end_point.x - flick_start_point.x;
            int32_t dy = end_point.y - flick_start_point.y;

            flick_direction_t dir = calc_flick_direction(dx, dy);

            const char *romaji = flick_to_romaji((kana_row_t)row, dir);
            if (romaji && romaji[0] != '\0') {
                // Apply modifier if set, then send
                apply_modifier_and_send(romaji, (kana_row_t)row);
            }
        }
        flick_active_panel = NULL;
    }
}

/**
 * @brief Handle JPKeyboard special key press (Backspace, arrows, space, enter)
 * Events are attached to images/labels inside panels for reliable click detection
 */
static void jp_special_key_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    lv_obj_t *obj = lv_event_get_current_target(e);
    uint8_t keycode = 0;

    if (obj == ui_Image3) {
        keycode = HID_KEY_BACKSPACE;
    } else if (obj == ui_Image4) {
        keycode = HID_KEY_LEFT;
    } else if (obj == ui_Image1) {
        keycode = HID_KEY_RIGHT;
    } else if (obj == ui_Label11) {
        keycode = HID_KEY_SPACE;
    } else if (obj == ui_Image5) {
        keycode = HID_KEY_ENTER;
    }

    if (keycode != 0) {
        ble_hid_send_key(0, keycode);
    }
}

/**
 * @brief Handle symbol panel (Panel20) input with flick
 * 中央=読点(、)、上=？、左=句点(。)、下=…、右=！
 */
static lv_point_t symbol_start_point = {0, 0};
static bool symbol_touch_active = false;

static void symbol_panel_input_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &symbol_start_point);
            symbol_touch_active = true;
        }
        return;
    }

    if (code == LV_EVENT_RELEASED && symbol_touch_active) {
        lv_point_t end_point;
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &end_point);

            int32_t dx = end_point.x - symbol_start_point.x;
            int32_t dy = end_point.y - symbol_start_point.y;

            flick_direction_t dir = calc_flick_direction(dx, dy);
            const char *symbol = NULL;

            switch (dir) {
            case FLICK_CENTER:
                symbol = ",";  // 読点 (、)
                break;
            case FLICK_UP:
                symbol = "?";
                break;
            case FLICK_LEFT:
                symbol = ".";  // 句点 (。)
                break;
            case FLICK_DOWN:
                ble_hid_send_string("...");
                symbol_touch_active = false;
                return;
            case FLICK_RIGHT:
                symbol = "!";
                break;
            default:
                break;
            }

            if (symbol) {
                ble_hid_send_string(symbol);
            }
        }
        symbol_touch_active = false;
    }
}

/**
 * @brief Handle modifier panel (Panel18) input with flick
 * 中央=小さくなる、左=濁点、右=半濁点
 */
static lv_point_t modifier_start_point = {0, 0};
static bool modifier_touch_active = false;

static void modifier_panel_input_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &modifier_start_point);
            modifier_touch_active = true;
        }
        return;
    }

    if (code == LV_EVENT_RELEASED && modifier_touch_active) {
        lv_point_t end_point;
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_indev_get_point(indev, &end_point);

            int32_t dx = end_point.x - modifier_start_point.x;
            int32_t dy = end_point.y - modifier_start_point.y;

            flick_direction_t dir = calc_flick_direction(dx, dy);

            switch (dir) {
            case FLICK_CENTER:
                current_modifier = MODIFIER_SMALL;
                break;
            case FLICK_LEFT:
                current_modifier = MODIFIER_DAKUTEN;
                break;
            case FLICK_RIGHT:
                current_modifier = MODIFIER_HANDAKUTEN;
                break;
            default:
                current_modifier = MODIFIER_NONE;
                break;
            }
        }
        modifier_touch_active = false;
    }
}

/**
 * @brief Handle cursor key press
 * Events are attached to images/labels inside panels for reliable touch detection
 */
static void cursor_key_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    lv_obj_t *obj = lv_event_get_current_target(e);
    uint8_t keycode = 0;
    uint8_t modifiers = 0;

    if (obj == ui_Label16) {
        keycode = HID_KEY_ESC;
    } else if (obj == ui_Label18) {
        keycode = HID_KEY_ENTER;
    } else if (obj == ui_Image14) {
        keycode = HID_KEY_UP;
    } else if (obj == ui_Image13) {
        keycode = HID_KEY_LEFT;
    } else if (obj == ui_Image15) {
        keycode = HID_KEY_DOWN;
    } else if (obj == ui_Image12) {
        keycode = HID_KEY_RIGHT;
    } else if (obj == ui_Label17) {
        keycode = HID_KEY_V;
        modifiers = HID_MOD_LCTRL;
    } else if (obj == ui_Label20) {
        keycode = HID_KEY_C;
        modifiers = HID_MOD_LCTRL;
    } else if (obj == ui_Label25) {
        keycode = HID_KEY_Z;
        modifiers = HID_MOD_LCTRL;
    } else if (obj == ui_Label21) {
        keycode = HID_KEY_Y;
        modifiers = HID_MOD_LCTRL;
    } else if (obj == ui_Label26) {
        keycode = HID_KEY_BACKSPACE;
    }

    if (keycode != 0) {
        ble_hid_send_key(modifiers, keycode);
    }
}

/**
 * @brief Handle AtoZ keyboard character output
 */
static void atoz_keyboard_char_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code != LV_EVENT_VALUE_CHANGED) return;

    uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
    if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;

    const char *txt = lv_buttonmatrix_get_button_text(kb, btn_id);
    if (!txt) return;

    // Single character keys
    if (strlen(txt) == 1) {
        ESP_LOGI(TAG, "AtoZ: '%s'", txt);
        ble_hid_send_string(txt);
    }
    // Special keys
    else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        ble_hid_send_key(0, HID_KEY_BACKSPACE);
    } else if (strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
        ble_hid_send_key(0, HID_KEY_ENTER);
    }
}

/**
 * @brief Keyboard value changed callback - detect mode switch buttons
 */
static void keyboard_value_changed_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code != LV_EVENT_VALUE_CHANGED) return;

    uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
    if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;

    // Check if mode switch button was pressed (Button ID 35)
    if (btn_id == 35) {
        ESP_LOGI(TAG, "AtoZ->JP (mode switch button)");
        _ui_screen_change(&ui_JPKeyboardScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_JPKeyboardScreen_screen_init);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

static void setup_panel_nav(lv_obj_t *panel, lv_event_cb_t cb, const char *name)
{
    if (panel) {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(panel, cb, LV_EVENT_RELEASED, NULL);
        ESP_LOGD(TAG, "Nav: %s", name);
    }
}

// ============================================================================
// Screen-Specific Navigation Setup
// ============================================================================

static void setup_kana_panel_input(lv_obj_t *panel, const char *name)
{
    if (panel) {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(panel, kana_panel_input_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(panel, kana_panel_input_cb, LV_EVENT_RELEASED, NULL);
        ESP_LOGD(TAG, "Kana: %s", name);
    }
}

static void setup_jp_keyboard_nav(void)
{

    // Navigation buttons
    setup_panel_nav(ui_Image2, nav_to_menu, "JP:Image2->Menu");
    setup_panel_nav(ui_Panel17, nav_to_atoz_keyboard, "JP:Panel17->AtoZ");
    setup_panel_nav(ui_Panel12, nav_to_cursor, "JP:Panel12->Cursor");

    // Kana input panels
    setup_kana_panel_input(ui_Panel3, "あ");
    setup_kana_panel_input(ui_Panel4, "か");
    setup_kana_panel_input(ui_Panel5, "さ");
    setup_kana_panel_input(ui_Panel8, "た");
    setup_kana_panel_input(ui_Panel9, "な");
    setup_kana_panel_input(ui_Panel10, "は");
    setup_kana_panel_input(ui_Panel13, "ま");
    setup_kana_panel_input(ui_Panel14, "や");
    setup_kana_panel_input(ui_Panel15, "ら");
    setup_kana_panel_input(ui_Panel19, "わ");

    // Special keys - attach events to images/labels inside panels
    if (ui_Image3) {
        lv_obj_add_flag(ui_Image3, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Image3, jp_special_key_cb, LV_EVENT_RELEASED, NULL);
    }
    if (ui_Image4) {
        lv_obj_add_flag(ui_Image4, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Image4, jp_special_key_cb, LV_EVENT_RELEASED, NULL);
    }
    if (ui_Image1) {
        lv_obj_add_flag(ui_Image1, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Image1, jp_special_key_cb, LV_EVENT_RELEASED, NULL);
    }
    if (ui_Label11) {
        lv_obj_add_flag(ui_Label11, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Label11, jp_special_key_cb, LV_EVENT_RELEASED, NULL);
    }
    if (ui_Image5) {
        lv_obj_add_flag(ui_Image5, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Image5, jp_special_key_cb, LV_EVENT_RELEASED, NULL);
    }

    // Modifier panel (Panel18): 小さくなる/濁点/半濁点
    if (ui_Panel18) {
        lv_obj_add_flag(ui_Panel18, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Panel18, modifier_panel_input_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(ui_Panel18, modifier_panel_input_cb, LV_EVENT_RELEASED, NULL);
        ESP_LOGD(TAG, "JP: Modifier panel");
    }

    // Symbol panel with flick support (、？。…！)
    if (ui_Panel20) {
        lv_obj_add_flag(ui_Panel20, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Panel20, symbol_panel_input_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(ui_Panel20, symbol_panel_input_cb, LV_EVENT_RELEASED, NULL);
        ESP_LOGD(TAG, "JP: Symbol panel");
    }

    // Touchpad setup
    setup_touchpad_panel(ui_TouchAndScrollPanel, ui_Switch1);

    last_jp_screen = ui_JPKeyboardScreen;
    ESP_LOGD(TAG, "JPKeyboard nav ready");
}

static void setup_atoz_keyboard_nav(void)
{
    setup_panel_nav(ui_HeaderPanel1, nav_to_jp_keyboard, "AtoZ:Header->JP");
    if (ui_OtherKeyboard) {
        // Mode switch (button 35)
        lv_obj_add_event_cb(ui_OtherKeyboard, keyboard_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
        // Character output
        lv_obj_add_event_cb(ui_OtherKeyboard, atoz_keyboard_char_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // Touchpad setup
    setup_touchpad_panel(ui_TouchAndScrollPanel1, ui_Switch2);

    last_atoz_screen = ui_AtoZKeyboardScreen;
    ESP_LOGD(TAG, "AtoZ nav ready");
}

static void setup_cursor_key(lv_obj_t *obj, const char *name)
{
    if (obj) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(obj, cursor_key_cb, LV_EVENT_RELEASED, NULL);
        ESP_LOGD(TAG, "CursorKey: %s", name);
    }
}

static void setup_cursor_nav(void)
{
    // Navigation buttons - attach to images/labels inside panels
    setup_panel_nav(ui_Image6, nav_to_menu, "Cursor:Image6->Menu");
    setup_panel_nav(ui_Label22, nav_to_atoz_keyboard, "Cursor:Label22->AtoZ");
    setup_panel_nav(ui_Label27, nav_to_jp_keyboard, "Cursor:Label27->JP");

    // Cursor keys - attach to images/labels inside panels
    setup_cursor_key(ui_Label16, "ESC");
    setup_cursor_key(ui_Label18, "Enter");
    setup_cursor_key(ui_Image14, "Up");
    setup_cursor_key(ui_Image13, "Left");
    setup_cursor_key(ui_Image15, "Down");
    setup_cursor_key(ui_Image12, "Right");
    setup_cursor_key(ui_Label17, "Paste");
    setup_cursor_key(ui_Label20, "Copy");
    setup_cursor_key(ui_Label25, "Undo");
    setup_cursor_key(ui_Label21, "Redo");
    setup_cursor_key(ui_Label26, "Backspace");

    // Touchpad setup
    setup_touchpad_panel(ui_TouchAndScrollPanel2, ui_Switch3);

    last_cursor_screen = ui_CursorScreen;
    ESP_LOGD(TAG, "Cursor nav ready");
}

static void setup_clock_nav(void)
{
    setup_panel_nav(ui_Image11, nav_to_menu, "Clock:Image11->Menu");

    last_clock_screen = ui_AnalogClockWithBackgroud;
    ESP_LOGD(TAG, "Clock nav ready");
}

static void setup_datetime_nav(void)
{
    if (ui_Back) {
        lv_obj_add_flag(ui_Back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Back, nav_to_menu, LV_EVENT_RELEASED, NULL);
    }

    last_datetime_screen = ui_DateAndTime;
    ESP_LOGD(TAG, "DateTime nav ready");
}

// ============================================================================
// BLE Settings UI
// ============================================================================

/**
 * @brief Update BLE dropdown with bonded devices
 */
static void update_ble_dropdown(void)
{
    if (!ble_dropdown) return;

    // Get bonded devices
    g_bonded_count = ble_hid_get_bonded_devices(g_bonded_addrs, MAX_BONDED_DEVICES);

    // Build dropdown options string
    static char options[256];
    options[0] = '\0';

    if (g_bonded_count == 0) {
        strcpy(options, "(No devices)");
    } else {
        int offset = 0;
        for (int i = 0; i < g_bonded_count; i++) {
            if (i > 0) {
                options[offset++] = '\n';
            }
            offset += snprintf(options + offset, sizeof(options) - offset,
                              "%02X:%02X:%02X:%02X:%02X:%02X",
                              g_bonded_addrs[i][5], g_bonded_addrs[i][4],
                              g_bonded_addrs[i][3], g_bonded_addrs[i][2],
                              g_bonded_addrs[i][1], g_bonded_addrs[i][0]);
        }
    }

    lv_dropdown_set_options(ble_dropdown, options);
}

/**
 * @brief Update BLE status UI based on current state
 */
static void update_ble_status_ui(void)
{
    if (!ble_status_label || !ble_connect_label) return;

    ble_hid_state_t state = ble_hid_get_state();
    const char *status_text = "Unknown";
    const char *btn_text = "Connect";

    switch (state) {
    case BLE_HID_STATE_CONNECTED:
        status_text = "Connected";
        btn_text = "Disconnect";
        break;
    case BLE_HID_STATE_ADVERTISING:
        status_text = "Advertising...";
        btn_text = "Stop";
        break;
    case BLE_HID_STATE_DISCONNECTED:
    case BLE_HID_STATE_IDLE:
    default:
        status_text = "Disconnected";
        btn_text = "Connect";
        break;
    }

    lv_label_set_text(ble_status_label, status_text);
    lv_label_set_text(ble_connect_label, btn_text);

    // Update dropdown with bonded devices
    update_ble_dropdown();
}

/**
 * @brief BLE event callback (called from BLE task)
 */
static void ble_event_callback(ble_hid_state_t state)
{
    g_ble_ui_state = state;
    g_ble_state_changed = true;
    ESP_LOGI(TAG, "BLE event: state=%d", state);
}

/**
 * @brief Alarm event callback (called from alarm check)
 */
static void alarm_event_callback(alarm_event_t event, uint8_t index, void *user_data)
{
    (void)user_data;

    if (event == ALARM_EVENT_TRIGGERED) {
        g_alarm_triggered = true;
        g_alarm_triggered_index = index;
        ESP_LOGI(TAG, "Alarm event: triggered, index=%d", index);
    } else if (event == ALARM_EVENT_DISMISSED || event == ALARM_EVENT_SNOOZED ||
               event == ALARM_EVENT_EXPIRED) {
        g_alarm_triggered = false;
        g_alarm_triggered_index = 0xFF;
        ESP_LOGI(TAG, "Alarm event: %s, index=%d",
                 event == ALARM_EVENT_DISMISSED ? "dismissed" :
                 event == ALARM_EVENT_SNOOZED ? "snoozed" : "expired", index);
    }
}

/**
 * @brief Connect/disconnect button callback
 */
static void ble_connect_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    ble_hid_state_t state = ble_hid_get_state();

    if (state == BLE_HID_STATE_CONNECTED) {
        ESP_LOGI(TAG, "Disconnecting BLE...");
        ble_hid_disconnect();
    } else if (state == BLE_HID_STATE_ADVERTISING) {
        ESP_LOGI(TAG, "Stopping advertising...");
        ble_hid_stop_advertising();
    } else {
        ESP_LOGI(TAG, "Starting advertising...");
        ble_hid_start_advertising();
    }

    // Update UI immediately
    update_ble_status_ui();
}

/**
 * @brief Delete selected bond button callback
 */
static void ble_delete_bond_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (!ble_dropdown || g_bonded_count == 0) return;

    uint32_t selected = lv_dropdown_get_selected(ble_dropdown);
    if (selected < (uint32_t)g_bonded_count) {
        ESP_LOGI(TAG, "Deleting bond %lu...", (unsigned long)selected);
        esp_err_t ret = ble_hid_delete_bond(g_bonded_addrs[selected]);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Bond deleted successfully");
        } else {
            ESP_LOGW(TAG, "Failed to delete bond: %s", esp_err_to_name(ret));
        }
        update_ble_status_ui();
    }
}

/**
 * @brief Clear all bonds button callback
 */
static void ble_clear_bonds_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    ESP_LOGI(TAG, "Clearing all bonds...");
    esp_err_t ret = ble_hid_delete_all_bonds();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Bonds cleared successfully");
    } else {
        ESP_LOGW(TAG, "Failed to clear bonds: %s", esp_err_to_name(ret));
    }

    // Update UI
    update_ble_status_ui();
}

// ============================================================================
// Background Settings Functions
// ============================================================================

/**
 * @brief Scan background files from SD card and update dropdown options
 */
static void update_bg_dropdown_options(void)
{
    if (!sdcard_is_available()) {
        s_bg_file_count = 0;
        return;
    }

    esp_err_t ret = sdcard_list_backgrounds(s_bg_files, MAX_BG_FILES, &s_bg_file_count);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to list backgrounds: %s", esp_err_to_name(ret));
        s_bg_file_count = 0;
    }

    ESP_LOGI(TAG, "Found %zu background files", s_bg_file_count);
}

/**
 * @brief Build dropdown options string from file list
 */
static void build_bg_options_string(char *buf, size_t buf_len, bool include_default)
{
    int offset = 0;

    // First option
    if (include_default) {
        offset += snprintf(buf + offset, buf_len - offset, "Default");
    } else {
        offset += snprintf(buf + offset, buf_len - offset, "None");
    }

    // Add files
    for (size_t i = 0; i < s_bg_file_count && offset < (int)buf_len - 1; i++) {
        offset += snprintf(buf + offset, buf_len - offset, "\n%s", s_bg_files[i].filename);
    }
}

/**
 * @brief Find dropdown index for current background path
 */
static uint32_t find_bg_index(const char *current_path)
{
    if (!current_path || current_path[0] == '\0') {
        return 0;  // None/Default
    }

    for (size_t i = 0; i < s_bg_file_count; i++) {
        if (strcmp(s_bg_files[i].full_path, current_path) == 0) {
            return i + 1;  // +1 because index 0 is None/Default
        }
    }

    return 0;
}

/**
 * @brief Touchpad background dropdown callback
 */
static void bg_touchpad_changed_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    uint32_t idx = lv_dropdown_get_selected(bg_touchpad_dropdown);

    if (idx == 0) {
        // None selected
        sdcard_set_touchpad_bg(SDCARD_BG_NONE);
        ui_bg_clear_touchpad();
        ESP_LOGI(TAG, "Touchpad background cleared");
    } else if (idx <= s_bg_file_count) {
        const char *path = s_bg_files[idx - 1].full_path;
        sdcard_set_touchpad_bg(path);
        ui_bg_apply_to_touchpad(path);
        ESP_LOGI(TAG, "Touchpad background set: %s", path);
    }
}

/**
 * @brief Clock background dropdown callback
 */
static void bg_clock_changed_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    uint32_t idx = lv_dropdown_get_selected(bg_clock_dropdown);

    if (idx == 0) {
        // Default selected
        sdcard_set_clock_bg(SDCARD_BG_NONE);
        ui_bg_clear_clock();
        ESP_LOGI(TAG, "Clock background set to default");
    } else if (idx <= s_bg_file_count) {
        const char *path = s_bg_files[idx - 1].full_path;
        sdcard_set_clock_bg(path);
        ui_bg_apply_to_clock(path);
        ESP_LOGI(TAG, "Clock background set: %s", path);
    }
}

/**
 * @brief Create background settings UI
 */
static void create_bg_settings_ui(lv_obj_t *parent)
{
    // Scan files first
    update_bg_dropdown_options();

    // Background Settings Title
    lv_obj_t *bg_title = lv_label_create(parent);
    lv_label_set_text(bg_title, "Background Settings");
    lv_obj_set_style_text_font(bg_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(bg_title, lv_color_hex(0xFFFFFF), 0);

    // SD card status
    lv_obj_t *sd_status = lv_label_create(parent);
    if (sdcard_is_available()) {
        char status_text[64];
        snprintf(status_text, sizeof(status_text), "SD Card: %zu files found", s_bg_file_count);
        lv_label_set_text(sd_status, status_text);
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(sd_status, "SD Card: Not available");
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0xFF6666), 0);
    }
    lv_obj_set_style_text_font(sd_status, &lv_font_montserrat_14, 0);

    // Touchpad background label
    lv_obj_t *tp_label = lv_label_create(parent);
    lv_label_set_text(tp_label, "Touchpad Background:");
    lv_obj_set_style_text_font(tp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tp_label, lv_color_hex(0xFFFFFF), 0);

    // Touchpad background dropdown
    bg_touchpad_dropdown = lv_dropdown_create(parent);
    lv_obj_set_width(bg_touchpad_dropdown, 280);
    lv_obj_set_style_text_font(bg_touchpad_dropdown, &lv_font_montserrat_14, 0);

    static char tp_options[1024];
    build_bg_options_string(tp_options, sizeof(tp_options), false);
    lv_dropdown_set_options(bg_touchpad_dropdown, tp_options);

    // Select current touchpad background
    char current_tp_bg[128] = {0};
    if (sdcard_get_touchpad_bg(current_tp_bg, sizeof(current_tp_bg)) == ESP_OK) {
        lv_dropdown_set_selected(bg_touchpad_dropdown, find_bg_index(current_tp_bg));
    }

    lv_obj_add_event_cb(bg_touchpad_dropdown, bg_touchpad_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Clock background label
    lv_obj_t *clock_label = lv_label_create(parent);
    lv_label_set_text(clock_label, "Clock Background:");
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(clock_label, lv_color_hex(0xFFFFFF), 0);

    // Clock background dropdown
    bg_clock_dropdown = lv_dropdown_create(parent);
    lv_obj_set_width(bg_clock_dropdown, 280);
    lv_obj_set_style_text_font(bg_clock_dropdown, &lv_font_montserrat_14, 0);

    static char clock_options[1024];
    build_bg_options_string(clock_options, sizeof(clock_options), true);
    lv_dropdown_set_options(bg_clock_dropdown, clock_options);

    // Select current clock background
    char current_clock_bg[128] = {0};
    if (sdcard_get_clock_bg(current_clock_bg, sizeof(current_clock_bg)) == ESP_OK) {
        lv_dropdown_set_selected(bg_clock_dropdown, find_bg_index(current_clock_bg));
    }

    lv_obj_add_event_cb(bg_clock_dropdown, bg_clock_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    ESP_LOGI(TAG, "Background settings UI created");
}

static void setup_settings_nav(void)
{
    // Create elements only if screen changed (destroyed and recreated)
    if (ui_SettingScreen != last_settings_screen) {
        setting_back_btn = NULL;
        ble_status_label = NULL;
        ble_dropdown = NULL;
        ble_connect_btn = NULL;
        ble_connect_label = NULL;
        ble_delete_btn = NULL;
        ble_clear_btn = NULL;
        bg_touchpad_dropdown = NULL;
        bg_clock_dropdown = NULL;
        bg_ui_created = false;
    }

    if (ui_SettingScreen && !setting_back_btn) {
        // Back button
        setting_back_btn = lv_image_create(ui_SettingScreen);
        lv_image_set_src(setting_back_btn, &ui_img_2026807732);
        lv_obj_set_width(setting_back_btn, LV_SIZE_CONTENT);
        lv_obj_set_height(setting_back_btn, LV_SIZE_CONTENT);
        lv_obj_set_align(setting_back_btn, LV_ALIGN_BOTTOM_RIGHT);
        lv_obj_set_x(setting_back_btn, -30);
        lv_obj_set_y(setting_back_btn, -30);
        lv_obj_add_flag(setting_back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(setting_back_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(setting_back_btn, nav_to_menu, LV_EVENT_RELEASED, NULL);
    }

    // Create BLE settings UI in ui_Panel43 (settings content area)
    if (ui_Panel43 && !ble_status_label) {
        // Register BLE event callback
        ble_hid_register_callback(ble_event_callback);

        // Configure Panel43 for vertical layout with scrolling
        lv_obj_set_flex_flow(ui_Panel43, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(ui_Panel43, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(ui_Panel43, 12, 0);
        lv_obj_set_style_pad_top(ui_Panel43, 15, 0);
        lv_obj_set_style_pad_bottom(ui_Panel43, 30, 0);  // Bottom padding for scroll
        lv_obj_add_flag(ui_Panel43, LV_OBJ_FLAG_SCROLLABLE);  // Enable scrolling

        // BLE Section Title
        lv_obj_t *ble_title = lv_label_create(ui_Panel43);
        lv_label_set_text(ble_title, "Bluetooth Settings");
        lv_obj_set_style_text_font(ble_title, &lv_font_montserrat_18, 0);

        // Status label
        ble_status_label = lv_label_create(ui_Panel43);
        lv_label_set_text(ble_status_label, "Checking...");
        lv_obj_set_style_text_font(ble_status_label, &lv_font_montserrat_16, 0);

        // Bonded devices dropdown label
        lv_obj_t *dropdown_label = lv_label_create(ui_Panel43);
        lv_label_set_text(dropdown_label, "Paired Devices:");
        lv_obj_set_style_text_font(dropdown_label, &lv_font_montserrat_14, 0);

        // Dropdown for device selection
        ble_dropdown = lv_dropdown_create(ui_Panel43);
        lv_obj_set_width(ble_dropdown, 220);
        lv_dropdown_set_options(ble_dropdown, "(No devices)");
        lv_obj_set_style_text_font(ble_dropdown, &lv_font_montserrat_14, 0);

        // Button container (horizontal)
        lv_obj_t *btn_container = lv_obj_create(ui_Panel43);
        lv_obj_set_size(btn_container, 240, 50);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(btn_container, 0, 0);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);

        // Connect/Disconnect button
        ble_connect_btn = lv_button_create(btn_container);
        lv_obj_set_size(ble_connect_btn, 110, 40);
        lv_obj_add_flag(ble_connect_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ble_connect_btn, ble_connect_btn_cb, LV_EVENT_RELEASED, NULL);
        lv_obj_set_style_bg_color(ble_connect_btn, lv_color_hex(0x2196F3), 0);

        ble_connect_label = lv_label_create(ble_connect_btn);
        lv_label_set_text(ble_connect_label, "Connect");
        lv_obj_center(ble_connect_label);
        lv_obj_set_style_text_font(ble_connect_label, &lv_font_montserrat_14, 0);

        // Delete selected button
        ble_delete_btn = lv_button_create(btn_container);
        lv_obj_set_size(ble_delete_btn, 110, 40);
        lv_obj_add_flag(ble_delete_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ble_delete_btn, ble_delete_bond_cb, LV_EVENT_RELEASED, NULL);
        lv_obj_set_style_bg_color(ble_delete_btn, lv_color_hex(0xFF9800), 0);

        lv_obj_t *delete_label = lv_label_create(ble_delete_btn);
        lv_label_set_text(delete_label, "Delete");
        lv_obj_center(delete_label);
        lv_obj_set_style_text_font(delete_label, &lv_font_montserrat_14, 0);

        // Clear all bonds button
        ble_clear_btn = lv_button_create(ui_Panel43);
        lv_obj_set_size(ble_clear_btn, 180, 40);
        lv_obj_add_flag(ble_clear_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ble_clear_btn, ble_clear_bonds_cb, LV_EVENT_RELEASED, NULL);
        lv_obj_set_style_bg_color(ble_clear_btn, lv_color_hex(0xF44336), 0);

        lv_obj_t *clear_label = lv_label_create(ble_clear_btn);
        lv_label_set_text(clear_label, "Clear All");
        lv_obj_center(clear_label);
        lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_14, 0);

        // Initial UI update
        update_ble_status_ui();

        ESP_LOGI(TAG, "BLE settings UI created");
    }

    // Create Alarm settings UI (below BLE settings)
    static bool alarm_ui_created = false;
    if (ui_Panel43 && !alarm_ui_created) {
        // Add separator
        lv_obj_t *separator = lv_obj_create(ui_Panel43);
        lv_obj_set_size(separator, 200, 2);
        lv_obj_set_style_bg_color(separator, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(separator, 0, 0);
        lv_obj_set_style_pad_all(separator, 0, 0);

        // Initialize alarm UI in the settings panel
        ui_alarm_init(ui_Panel43);
        alarm_ui_created = true;

        ESP_LOGI(TAG, "Alarm settings UI created");
    }

    // Create Background settings UI (below Alarm settings)
    if (ui_Panel43 && !bg_ui_created) {
        // Add separator
        lv_obj_t *separator2 = lv_obj_create(ui_Panel43);
        lv_obj_set_size(separator2, 200, 2);
        lv_obj_set_style_bg_color(separator2, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(separator2, 0, 0);
        lv_obj_set_style_pad_all(separator2, 0, 0);

        // Initialize background settings UI
        create_bg_settings_ui(ui_Panel43);
        bg_ui_created = true;
    }

    last_settings_screen = ui_SettingScreen;
    ESP_LOGD(TAG, "Settings nav ready");
}

// ============================================================================
// Update Function
// ============================================================================

void ui_navigation_update(void)
{
    // Check if screen objects have been recreated (different pointer = new object)
    // This handles the case where screen_destroy() is called and screen_init() creates new objects
    if (ui_JPKeyboardScreen && ui_JPKeyboardScreen != last_jp_screen) {
        setup_jp_keyboard_nav();
    }

    if (ui_AtoZKeyboardScreen && ui_AtoZKeyboardScreen != last_atoz_screen) {
        setup_atoz_keyboard_nav();
    }

    if (ui_CursorScreen && ui_CursorScreen != last_cursor_screen) {
        setup_cursor_nav();
    }

    if (ui_AnalogClockWithBackgroud && ui_AnalogClockWithBackgroud != last_clock_screen) {
        setup_clock_nav();
    }

    if (ui_DateAndTime && ui_DateAndTime != last_datetime_screen) {
        setup_datetime_nav();
    }

    if (ui_SettingScreen && ui_SettingScreen != last_settings_screen) {
        setup_settings_nav();
    }
}

static void nav_timer_cb(lv_timer_t *timer)
{
    ui_navigation_update();

    // Check for BLE state changes and update UI (thread-safe via LVGL timer)
    if (g_ble_state_changed) {
        g_ble_state_changed = false;
        update_ble_status_ui();
    }

    // Check for alarm trigger and show popup (thread-safe via LVGL timer)
    if (g_alarm_triggered && !ui_alarm_is_popup_visible()) {
        ui_alarm_show_trigger_popup(g_alarm_triggered_index);
    }
}

// ============================================================================
// Initialization
// ============================================================================

void ui_navigation_init(void)
{
    // Menu Screen navigation - use LV_EVENT_RELEASED for reliable touch detection
    if (ui_KeyBoards) {
        lv_obj_add_flag(ui_KeyBoards, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_KeyBoards, nav_to_jp_keyboard, LV_EVENT_RELEASED, NULL);
    }

    if (ui_Clock) {
        lv_obj_add_flag(ui_Clock, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Clock, nav_to_clock, LV_EVENT_RELEASED, NULL);
    }

    if (ui_PowerOnPC) {
        lv_obj_add_flag(ui_PowerOnPC, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_PowerOnPC, nav_power_on_pc, LV_EVENT_RELEASED, NULL);
    }

    if (ui_LightOn) {
        lv_obj_add_flag(ui_LightOn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_LightOn, nav_light_on, LV_EVENT_RELEASED, NULL);
    }

    if (ui_Setthing) {
        lv_obj_add_flag(ui_Setthing, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Setthing, nav_to_settings, LV_EVENT_RELEASED, NULL);
    }

    // Timer for lazy screen navigation setup
    lv_timer_create(nav_timer_cb, 100, NULL);

    // Register alarm event callback for UI updates
    alarm_register_callback(alarm_event_callback, NULL);

    ESP_LOGI(TAG, "Navigation initialized");
}
