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

// BLE HID
#include "ble_hid.h"
#include "ble_hid_keycodes.h"
#include "flick_input.h"

static const char *TAG = "UI_NAV";

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

// CursorScreen buttons
extern lv_obj_t *ui_Panel1;         // Container for Image6
extern lv_obj_t *ui_Image6;         // Top-left (絵) -> Menu (actual clickable image)
extern lv_obj_t *ui_Panel31;        // [a] -> AtoZ
extern lv_obj_t *ui_Panel36;        // あ -> JP Keyboard

// CursorScreen key panels
extern lv_obj_t *ui_Panel22;        // ESC
extern lv_obj_t *ui_Panel24;        // Enter
extern lv_obj_t *ui_Panel37;        // Up arrow
extern lv_obj_t *ui_Panel38;        // Left arrow
extern lv_obj_t *ui_Panel39;        // Down arrow
extern lv_obj_t *ui_Panel33;        // Right arrow
extern lv_obj_t *ui_Panel23;        // Paste (Ctrl+V)
extern lv_obj_t *ui_Panel28;        // Copy (Ctrl+C)
extern lv_obj_t *ui_Panel34;        // Undo (Ctrl+Z)
extern lv_obj_t *ui_Panel29;        // Redo (Ctrl+Y)
extern lv_obj_t *ui_Panel35;        // Backspace

// AnalogClockWithBackgroud back button
extern lv_obj_t *ui_Image11;

// DateAndTime back label
extern lv_obj_t *ui_Back;

// SettingScreen elements
extern lv_obj_t *ui_Panel43;

// Back button image declaration
LV_IMG_DECLARE(ui_img_2026807732);  // Uターン矢印 2.png

// Track which screens have been initialized with navigation
static bool nav_initialized_jp = false;
static bool nav_initialized_atoz = false;
static bool nav_initialized_cursor = false;
static bool nav_initialized_clock = false;
static bool nav_initialized_datetime = false;
static bool nav_initialized_settings = false;

// Dynamic elements
static lv_obj_t *setting_back_btn = NULL;

// ============================================================================
// Navigation Callbacks
// ============================================================================

static void nav_to_menu(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to Menu");
        _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_MenuScreen_screen_init);
    }
}

static void nav_to_jp_keyboard(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to JP Keyboard");
        _ui_screen_change(&ui_JPKeyboardScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_JPKeyboardScreen_screen_init);
    }
}

static void nav_to_atoz_keyboard(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to AtoZ Keyboard");
        _ui_screen_change(&ui_AtoZKeyboardScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, ui_AtoZKeyboardScreen_screen_init);
    }
}

static void nav_to_cursor(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to Cursor Screen");
        _ui_screen_change(&ui_CursorScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, ui_CursorScreen_screen_init);
    }
}

static void nav_to_clock(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to Analog Clock");
        _ui_screen_change(&ui_AnalogClockWithBackgroud, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_AnalogClockWithBackgroud_screen_init);
    }
}

static void nav_to_settings(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Navigating to Settings");
        _ui_screen_change(&ui_SettingScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, ui_SettingScreen_screen_init);
    }
}

static void nav_power_on_pc(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Power On PC clicked (WoL - not implemented yet)");
    }
}

static void nav_light_on(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Light On clicked (not implemented yet)");
    }
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
 */
static void cursor_key_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_obj_t *panel = lv_event_get_target(e);
    uint8_t keycode = 0;
    uint8_t modifiers = 0;

    if (panel == ui_Panel22) {
        keycode = HID_KEY_ESC;
        ESP_LOGI(TAG, "Cursor: ESC");
    } else if (panel == ui_Panel24) {
        keycode = HID_KEY_ENTER;
        ESP_LOGI(TAG, "Cursor: Enter");
    } else if (panel == ui_Panel37) {
        keycode = HID_KEY_UP;
        ESP_LOGI(TAG, "Cursor: Up");
    } else if (panel == ui_Panel38) {
        keycode = HID_KEY_LEFT;
        ESP_LOGI(TAG, "Cursor: Left");
    } else if (panel == ui_Panel39) {
        keycode = HID_KEY_DOWN;
        ESP_LOGI(TAG, "Cursor: Down");
    } else if (panel == ui_Panel33) {
        keycode = HID_KEY_RIGHT;
        ESP_LOGI(TAG, "Cursor: Right");
    } else if (panel == ui_Panel23) {
        keycode = HID_KEY_V;
        modifiers = HID_MOD_LCTRL;
        ESP_LOGI(TAG, "Cursor: Paste");
    } else if (panel == ui_Panel28) {
        keycode = HID_KEY_C;
        modifiers = HID_MOD_LCTRL;
        ESP_LOGI(TAG, "Cursor: Copy");
    } else if (panel == ui_Panel34) {
        keycode = HID_KEY_Z;
        modifiers = HID_MOD_LCTRL;
        ESP_LOGI(TAG, "Cursor: Undo");
    } else if (panel == ui_Panel29) {
        keycode = HID_KEY_Y;
        modifiers = HID_MOD_LCTRL;
        ESP_LOGI(TAG, "Cursor: Redo");
    } else if (panel == ui_Panel35) {
        keycode = HID_KEY_BACKSPACE;
        ESP_LOGI(TAG, "Cursor: Backspace");
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
        lv_obj_add_event_cb(panel, cb, LV_EVENT_CLICKED, NULL);
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
    if (nav_initialized_jp) return;

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

    nav_initialized_jp = true;
    ESP_LOGD(TAG, "JPKeyboard nav ready");
}

static void setup_atoz_keyboard_nav(void)
{
    if (nav_initialized_atoz) return;

    setup_panel_nav(ui_HeaderPanel1, nav_to_jp_keyboard, "AtoZ:Header->JP");
    if (ui_OtherKeyboard) {
        // Mode switch (button 35)
        lv_obj_add_event_cb(ui_OtherKeyboard, keyboard_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
        // Character output
        lv_obj_add_event_cb(ui_OtherKeyboard, atoz_keyboard_char_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    nav_initialized_atoz = true;
    ESP_LOGD(TAG, "AtoZ nav ready");
}

static void setup_cursor_key(lv_obj_t *panel, const char *name)
{
    if (panel) {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(panel, cursor_key_cb, LV_EVENT_CLICKED, NULL);
        ESP_LOGD(TAG, "CursorKey: %s", name);
    }
}

static void setup_cursor_nav(void)
{
    if (nav_initialized_cursor) return;

    // Navigation buttons
    setup_panel_nav(ui_Image6, nav_to_menu, "Cursor:Image6->Menu");
    setup_panel_nav(ui_Panel31, nav_to_atoz_keyboard, "Cursor:Panel31->AtoZ");
    setup_panel_nav(ui_Panel36, nav_to_jp_keyboard, "Cursor:Panel36->JP");

    // Cursor keys
    setup_cursor_key(ui_Panel22, "ESC");
    setup_cursor_key(ui_Panel24, "Enter");
    setup_cursor_key(ui_Panel37, "Up");
    setup_cursor_key(ui_Panel38, "Left");
    setup_cursor_key(ui_Panel39, "Down");
    setup_cursor_key(ui_Panel33, "Right");
    setup_cursor_key(ui_Panel23, "Paste");
    setup_cursor_key(ui_Panel28, "Copy");
    setup_cursor_key(ui_Panel34, "Undo");
    setup_cursor_key(ui_Panel29, "Redo");
    setup_cursor_key(ui_Panel35, "Backspace");

    nav_initialized_cursor = true;
    ESP_LOGD(TAG, "Cursor nav ready");
}

static void setup_clock_nav(void)
{
    if (nav_initialized_clock) return;

    setup_panel_nav(ui_Image11, nav_to_menu, "Clock:Image11->Menu");

    nav_initialized_clock = true;
    ESP_LOGD(TAG, "Clock nav ready");
}

static void setup_datetime_nav(void)
{
    if (nav_initialized_datetime) return;

    if (ui_Back) {
        lv_obj_add_flag(ui_Back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Back, nav_to_menu, LV_EVENT_CLICKED, NULL);
    }

    nav_initialized_datetime = true;
    ESP_LOGD(TAG, "DateTime nav ready");
}

static void setup_settings_nav(void)
{
    if (nav_initialized_settings) return;

    if (ui_SettingScreen && !setting_back_btn) {
        setting_back_btn = lv_image_create(ui_SettingScreen);
        lv_image_set_src(setting_back_btn, &ui_img_2026807732);
        lv_obj_set_width(setting_back_btn, LV_SIZE_CONTENT);
        lv_obj_set_height(setting_back_btn, LV_SIZE_CONTENT);
        lv_obj_set_align(setting_back_btn, LV_ALIGN_BOTTOM_RIGHT);
        lv_obj_set_x(setting_back_btn, -30);
        lv_obj_set_y(setting_back_btn, -30);
        lv_obj_add_flag(setting_back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(setting_back_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(setting_back_btn, nav_to_menu, LV_EVENT_CLICKED, NULL);
    }

    nav_initialized_settings = true;
    ESP_LOGD(TAG, "Settings nav ready");
}

// ============================================================================
// Update Function
// ============================================================================

void ui_navigation_update(void)
{
    if (ui_JPKeyboardScreen && !nav_initialized_jp) {
        setup_jp_keyboard_nav();
    }

    if (ui_AtoZKeyboardScreen && !nav_initialized_atoz) {
        setup_atoz_keyboard_nav();
    }

    if (ui_CursorScreen && !nav_initialized_cursor) {
        setup_cursor_nav();
    }

    if (ui_AnalogClockWithBackgroud && !nav_initialized_clock) {
        setup_clock_nav();
    }

    if (ui_DateAndTime && !nav_initialized_datetime) {
        setup_datetime_nav();
    }

    if (ui_SettingScreen && !nav_initialized_settings) {
        setup_settings_nav();
    }
}

static void nav_timer_cb(lv_timer_t *timer)
{
    ui_navigation_update();
}

// ============================================================================
// Initialization
// ============================================================================

void ui_navigation_init(void)
{
    // Menu Screen navigation
    if (ui_KeyBoards) {
        lv_obj_add_flag(ui_KeyBoards, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_KeyBoards, nav_to_jp_keyboard, LV_EVENT_CLICKED, NULL);
    }

    if (ui_Clock) {
        lv_obj_add_flag(ui_Clock, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Clock, nav_to_clock, LV_EVENT_CLICKED, NULL);
    }

    if (ui_PowerOnPC) {
        lv_obj_add_flag(ui_PowerOnPC, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_PowerOnPC, nav_power_on_pc, LV_EVENT_CLICKED, NULL);
    }

    if (ui_LightOn) {
        lv_obj_add_flag(ui_LightOn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_LightOn, nav_light_on, LV_EVENT_CLICKED, NULL);
    }

    if (ui_Setthing) {
        lv_obj_add_flag(ui_Setthing, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_Setthing, nav_to_settings, LV_EVENT_CLICKED, NULL);
    }

    // Timer for lazy screen navigation setup
    lv_timer_create(nav_timer_cb, 100, NULL);

    ESP_LOGI(TAG, "Navigation initialized");
}
