/**
 * @file ui_alarm.c
 * @brief Alarm Settings UI Implementation
 */

#include "ui_alarm.h"
#include "alarm.h"
#include "sdcard.h"
#include "esp_log.h"
#include "bsp/jc4880p443c.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

static const char *TAG = "UI_ALARM";

// UI element pointers
static lv_obj_t *s_alarm_panel = NULL;
static lv_obj_t *s_alarm_title = NULL;
static lv_obj_t *s_alarm_list = NULL;
static lv_obj_t *s_add_btn = NULL;

// Edit dialog elements
static lv_obj_t *s_edit_modal = NULL;
static lv_obj_t *s_hour_spinbox = NULL;
static lv_obj_t *s_minute_spinbox = NULL;
static lv_obj_t *s_day_checkboxes[7] = {NULL};
static lv_obj_t *s_sound_dropdown = NULL;
static lv_obj_t *s_enable_switch = NULL;
static lv_obj_t *s_delete_btn = NULL;
static uint8_t s_editing_index = 0xFF;

// Trigger popup elements
static lv_obj_t *s_trigger_popup = NULL;
static lv_obj_t *s_trigger_time_label = NULL;
static uint8_t s_triggered_index = 0xFF;

// Sound file list
#define MAX_SOUND_FILES 16
static char s_sound_files[MAX_SOUND_FILES][64];
static int s_sound_file_count = 0;

// Day names
static const char *DAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// ============================================================================
// Sound File Scanner
// ============================================================================

static void scan_sound_files(void)
{
    s_sound_file_count = 0;

    // Add "Default" option first
    strcpy(s_sound_files[s_sound_file_count++], "(Default)");

    if (!sdcard_is_available()) {
        return;
    }

    DIR *dir = opendir("/sdcard/sounds");
    if (!dir) {
        ESP_LOGW(TAG, "Cannot open /sdcard/sounds");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && s_sound_file_count < MAX_SOUND_FILES) {
        if (entry->d_type == DT_DIR || entry->d_name[0] == '.') {
            continue;
        }

        // Check for .wav extension
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcasecmp(ext, ".wav") != 0) {
            continue;
        }

        // Skip files with names too long for our buffer
        if (strlen(entry->d_name) >= 63) {
            continue;
        }

        strncpy(s_sound_files[s_sound_file_count], entry->d_name, 63);
        s_sound_files[s_sound_file_count][63] = '\0';
        s_sound_file_count++;
    }

    closedir(dir);
    ESP_LOGI(TAG, "Found %d sound files", s_sound_file_count - 1);
}

static void update_sound_dropdown(void)
{
    if (!s_sound_dropdown) return;

    scan_sound_files();

    // Build options string
    static char options[512];
    options[0] = '\0';
    int offset = 0;

    for (int i = 0; i < s_sound_file_count; i++) {
        if (i > 0) {
            options[offset++] = '\n';
        }
        offset += snprintf(options + offset, sizeof(options) - offset, "%s", s_sound_files[i]);
    }

    lv_dropdown_set_options(s_sound_dropdown, options);
}

// ============================================================================
// Edit Dialog
// ============================================================================

static void close_edit_dialog(void)
{
    if (s_edit_modal) {
        lv_obj_delete(s_edit_modal);
        s_edit_modal = NULL;
        s_hour_spinbox = NULL;
        s_minute_spinbox = NULL;
        for (int i = 0; i < 7; i++) {
            s_day_checkboxes[i] = NULL;
        }
        s_sound_dropdown = NULL;
        s_enable_switch = NULL;
        s_delete_btn = NULL;
    }
    s_editing_index = 0xFF;
}

static void save_alarm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (s_editing_index >= ALARM_MAX_COUNT) return;

    alarm_entry_t alarm;
    memset(&alarm, 0, sizeof(alarm));

    // Get time
    alarm.hour = (uint8_t)lv_spinbox_get_value(s_hour_spinbox);
    alarm.minute = (uint8_t)lv_spinbox_get_value(s_minute_spinbox);

    // Get repeat days
    alarm.repeat_days = 0;
    for (int i = 0; i < 7; i++) {
        if (lv_obj_has_state(s_day_checkboxes[i], LV_STATE_CHECKED)) {
            alarm.repeat_days |= (1 << i);
        }
    }

    // Get sound file
    uint32_t sound_idx = lv_dropdown_get_selected(s_sound_dropdown);
    if (sound_idx == 0 || sound_idx >= (uint32_t)s_sound_file_count) {
        // Default sound
        alarm.sound_path[0] = '\0';
    } else {
        snprintf(alarm.sound_path, ALARM_SOUND_PATH_LEN, "/sdcard/sounds/%s",
                 s_sound_files[sound_idx]);
    }

    // Get enabled state
    alarm.enabled = lv_obj_has_state(s_enable_switch, LV_STATE_CHECKED);

    // Save alarm
    esp_err_t ret = alarm_set(s_editing_index, &alarm);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Alarm %d saved", s_editing_index);
    } else {
        ESP_LOGW(TAG, "Failed to save alarm: %s", esp_err_to_name(ret));
    }

    close_edit_dialog();
    ui_alarm_update_list();
}

static void cancel_edit_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    close_edit_dialog();
}

static void delete_alarm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (s_editing_index >= ALARM_MAX_COUNT) return;

    alarm_delete(s_editing_index);
    ESP_LOGI(TAG, "Alarm %d deleted", s_editing_index);

    close_edit_dialog();
    ui_alarm_update_list();
}

static void show_edit_dialog(uint8_t index)
{
    if (s_edit_modal) {
        close_edit_dialog();
    }

    s_editing_index = index;

    // Get current alarm data
    alarm_entry_t alarm;
    if (alarm_get(index, &alarm) != ESP_OK) {
        memset(&alarm, 0, sizeof(alarm));
        alarm.hour = 7;
        alarm.enabled = true;
    }

    // Create modal background
    s_edit_modal = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_edit_modal, 280, 420);
    lv_obj_center(s_edit_modal);
    lv_obj_set_style_bg_color(s_edit_modal, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(s_edit_modal, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(s_edit_modal, 2, 0);
    lv_obj_set_style_radius(s_edit_modal, 12, 0);
    lv_obj_set_style_pad_all(s_edit_modal, 15, 0);
    lv_obj_set_flex_flow(s_edit_modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_edit_modal, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_edit_modal, 10, 0);

    // Title
    lv_obj_t *title = lv_label_create(s_edit_modal);
    lv_label_set_text(title, index < ALARM_MAX_COUNT ? "Edit Alarm" : "New Alarm");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // Time row
    lv_obj_t *time_row = lv_obj_create(s_edit_modal);
    lv_obj_set_size(time_row, 250, 60);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_row, 0, 0);
    lv_obj_set_style_pad_all(time_row, 5, 0);
    lv_obj_set_style_pad_column(time_row, 10, 0);

    // Hour spinbox
    s_hour_spinbox = lv_spinbox_create(time_row);
    lv_spinbox_set_range(s_hour_spinbox, 0, 23);
    lv_spinbox_set_digit_format(s_hour_spinbox, 2, 0);
    lv_spinbox_set_value(s_hour_spinbox, alarm.hour);
    lv_obj_set_width(s_hour_spinbox, 70);
    lv_obj_set_style_text_font(s_hour_spinbox, &lv_font_montserrat_24, 0);

    // Colon
    lv_obj_t *colon = lv_label_create(time_row);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_24, 0);

    // Minute spinbox
    s_minute_spinbox = lv_spinbox_create(time_row);
    lv_spinbox_set_range(s_minute_spinbox, 0, 59);
    lv_spinbox_set_digit_format(s_minute_spinbox, 2, 0);
    lv_spinbox_set_value(s_minute_spinbox, alarm.minute);
    lv_obj_set_width(s_minute_spinbox, 70);
    lv_obj_set_style_text_font(s_minute_spinbox, &lv_font_montserrat_24, 0);

    // Days label
    lv_obj_t *days_label = lv_label_create(s_edit_modal);
    lv_label_set_text(days_label, "Repeat:");
    lv_obj_set_style_text_font(days_label, &lv_font_montserrat_14, 0);

    // Days row
    lv_obj_t *days_row = lv_obj_create(s_edit_modal);
    lv_obj_set_size(days_row, 250, 35);
    lv_obj_set_flex_flow(days_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(days_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(days_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(days_row, 0, 0);
    lv_obj_set_style_pad_all(days_row, 0, 0);

    for (int i = 0; i < 7; i++) {
        s_day_checkboxes[i] = lv_checkbox_create(days_row);
        lv_checkbox_set_text(s_day_checkboxes[i], DAY_NAMES[i]);
        lv_obj_set_style_text_font(s_day_checkboxes[i], &lv_font_montserrat_12, 0);
        if (alarm.repeat_days & (1 << i)) {
            lv_obj_add_state(s_day_checkboxes[i], LV_STATE_CHECKED);
        }
    }

    // Sound label
    lv_obj_t *sound_label = lv_label_create(s_edit_modal);
    lv_label_set_text(sound_label, "Sound:");
    lv_obj_set_style_text_font(sound_label, &lv_font_montserrat_14, 0);

    // Sound dropdown
    s_sound_dropdown = lv_dropdown_create(s_edit_modal);
    lv_obj_set_width(s_sound_dropdown, 230);
    lv_obj_set_style_text_font(s_sound_dropdown, &lv_font_montserrat_12, 0);
    update_sound_dropdown();

    // Select current sound file
    if (alarm.sound_path[0] != '\0') {
        const char *filename = strrchr(alarm.sound_path, '/');
        if (filename) {
            filename++;  // Skip '/'
            for (int i = 0; i < s_sound_file_count; i++) {
                if (strcmp(s_sound_files[i], filename) == 0) {
                    lv_dropdown_set_selected(s_sound_dropdown, i);
                    break;
                }
            }
        }
    }

    // Enable switch row
    lv_obj_t *enable_row = lv_obj_create(s_edit_modal);
    lv_obj_set_size(enable_row, 230, 40);
    lv_obj_set_flex_flow(enable_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(enable_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(enable_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(enable_row, 0, 0);
    lv_obj_set_style_pad_all(enable_row, 5, 0);

    lv_obj_t *enable_label = lv_label_create(enable_row);
    lv_label_set_text(enable_label, "Enabled");
    lv_obj_set_style_text_font(enable_label, &lv_font_montserrat_14, 0);

    s_enable_switch = lv_switch_create(enable_row);
    if (alarm.enabled) {
        lv_obj_add_state(s_enable_switch, LV_STATE_CHECKED);
    }

    // Button row
    lv_obj_t *btn_row = lv_obj_create(s_edit_modal);
    lv_obj_set_size(btn_row, 250, 50);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);

    // Save button
    lv_obj_t *save_btn = lv_button_create(btn_row);
    lv_obj_set_size(save_btn, 70, 38);
    lv_obj_add_event_cb(save_btn, save_alarm_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);

    // Cancel button
    lv_obj_t *cancel_btn = lv_button_create(btn_row);
    lv_obj_set_size(cancel_btn, 70, 38);
    lv_obj_add_event_cb(cancel_btn, cancel_edit_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x757575), 0);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);

    // Delete button
    s_delete_btn = lv_button_create(btn_row);
    lv_obj_set_size(s_delete_btn, 70, 38);
    lv_obj_add_event_cb(s_delete_btn, delete_alarm_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(s_delete_btn, lv_color_hex(0xF44336), 0);
    lv_obj_t *delete_label = lv_label_create(s_delete_btn);
    lv_label_set_text(delete_label, "Delete");
    lv_obj_center(delete_label);
}

// ============================================================================
// Alarm List
// ============================================================================

static void alarm_item_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    uint8_t *index_ptr = (uint8_t *)lv_event_get_user_data(e);
    if (index_ptr) {
        show_edit_dialog(*index_ptr);
    }
}

static void add_alarm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    // Find first unused slot
    alarm_entry_t alarms[ALARM_MAX_COUNT];
    alarm_get_all(alarms);

    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!alarms[i].enabled && alarms[i].hour == 7 && alarms[i].minute == 0 &&
            alarms[i].repeat_days == 0) {
            // Empty slot
            show_edit_dialog(i);
            return;
        }
    }

    // Find first disabled slot
    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!alarms[i].enabled) {
            show_edit_dialog(i);
            return;
        }
    }

    ESP_LOGW(TAG, "No free alarm slots");
}

// Storage for item indices (needed for callbacks)
static uint8_t s_item_indices[ALARM_MAX_COUNT];

void ui_alarm_update_list(void)
{
    if (!s_alarm_list) return;

    // Clear existing items
    lv_obj_clean(s_alarm_list);

    // Get all alarms
    alarm_entry_t alarms[ALARM_MAX_COUNT];
    alarm_get_all(alarms);

    for (uint8_t i = 0; i < ALARM_MAX_COUNT; i++) {
        // Only show alarms that are enabled or have been configured
        if (!alarms[i].enabled &&
            alarms[i].hour == 7 && alarms[i].minute == 0 &&
            alarms[i].repeat_days == 0) {
            continue;
        }

        // Create alarm item
        lv_obj_t *item = lv_obj_create(s_alarm_list);
        lv_obj_set_size(item, 210, 50);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x2D2D2D), 0);
        lv_obj_set_style_radius(item, 8, 0);
        lv_obj_set_style_pad_all(item, 8, 0);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);

        s_item_indices[i] = i;
        lv_obj_add_event_cb(item, alarm_item_cb, LV_EVENT_RELEASED, &s_item_indices[i]);

        // Time label
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", alarms[i].hour, alarms[i].minute);
        lv_obj_t *time_label = lv_label_create(item);
        lv_label_set_text(time_label, time_str);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
        if (!alarms[i].enabled) {
            lv_obj_set_style_text_color(time_label, lv_color_hex(0x888888), 0);
        }

        // Days indicator
        char days_str[24] = "";
        if (alarms[i].repeat_days == ALARM_REPEAT_ONCE) {
            strcpy(days_str, "Once");
        } else if (alarms[i].repeat_days == ALARM_REPEAT_EVERYDAY) {
            strcpy(days_str, "Daily");
        } else if (alarms[i].repeat_days == ALARM_REPEAT_WEEKDAYS) {
            strcpy(days_str, "M-F");
        } else if (alarms[i].repeat_days == ALARM_REPEAT_WEEKEND) {
            strcpy(days_str, "S-S");
        } else {
            int offset = 0;
            for (int d = 0; d < 7; d++) {
                if (alarms[i].repeat_days & (1 << d)) {
                    if (offset > 0) days_str[offset++] = ',';
                    days_str[offset++] = DAY_NAMES[d][0];
                }
            }
            days_str[offset] = '\0';
        }

        lv_obj_t *days_label = lv_label_create(item);
        lv_label_set_text(days_label, days_str);
        lv_obj_set_style_text_font(days_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(days_label, lv_color_hex(0xAAAAAA), 0);
    }

    ESP_LOGD(TAG, "Alarm list updated");
}

// ============================================================================
// Trigger Popup
// ============================================================================

static void dismiss_alarm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    alarm_dismiss();
    ui_alarm_hide_trigger_popup();
}

static void snooze_alarm_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    alarm_snooze();
    ui_alarm_hide_trigger_popup();
}

void ui_alarm_show_trigger_popup(uint8_t alarm_index)
{
    if (s_trigger_popup) {
        ui_alarm_hide_trigger_popup();
    }

    s_triggered_index = alarm_index;

    alarm_entry_t alarm;
    if (alarm_get(alarm_index, &alarm) != ESP_OK) {
        return;
    }

    // Create fullscreen popup
    s_trigger_popup = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_trigger_popup, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_trigger_popup);
    lv_obj_set_style_bg_color(s_trigger_popup, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(s_trigger_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_trigger_popup, 0, 0);
    lv_obj_set_flex_flow(s_trigger_popup, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_trigger_popup, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_trigger_popup, 30, 0);

    // Alarm icon
    lv_obj_t *icon = lv_label_create(s_trigger_popup);
    lv_label_set_text(icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFFD700), 0);

    // Time display
    s_trigger_time_label = lv_label_create(s_trigger_popup);
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", alarm.hour, alarm.minute);
    lv_label_set_text(s_trigger_time_label, time_str);
    lv_obj_set_style_text_font(s_trigger_time_label, &lv_font_montserrat_24, 0);

    // Alarm label
    lv_obj_t *label = lv_label_create(s_trigger_popup);
    lv_label_set_text(label, "ALARM");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    // Button container
    lv_obj_t *btn_container = lv_obj_create(s_trigger_popup);
    lv_obj_set_size(btn_container, 280, 60);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);

    // Snooze button
    lv_obj_t *snooze_btn = lv_button_create(btn_container);
    lv_obj_set_size(snooze_btn, 120, 50);
    lv_obj_add_event_cb(snooze_btn, snooze_alarm_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(snooze_btn, lv_color_hex(0xFF9800), 0);
    lv_obj_t *snooze_label = lv_label_create(snooze_btn);
    lv_label_set_text(snooze_label, "Snooze");
    lv_obj_set_style_text_font(snooze_label, &lv_font_montserrat_18, 0);
    lv_obj_center(snooze_label);

    // Dismiss button
    lv_obj_t *dismiss_btn = lv_button_create(btn_container);
    lv_obj_set_size(dismiss_btn, 120, 50);
    lv_obj_add_event_cb(dismiss_btn, dismiss_alarm_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(dismiss_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_t *dismiss_label = lv_label_create(dismiss_btn);
    lv_label_set_text(dismiss_label, "Dismiss");
    lv_obj_set_style_text_font(dismiss_label, &lv_font_montserrat_18, 0);
    lv_obj_center(dismiss_label);

    ESP_LOGI(TAG, "Showing alarm trigger popup for alarm %d", alarm_index);
}

void ui_alarm_hide_trigger_popup(void)
{
    if (s_trigger_popup) {
        lv_obj_delete(s_trigger_popup);
        s_trigger_popup = NULL;
        s_trigger_time_label = NULL;
    }
    s_triggered_index = 0xFF;
}

bool ui_alarm_is_popup_visible(void)
{
    return s_trigger_popup != NULL;
}

// ============================================================================
// Public API
// ============================================================================

void ui_alarm_init(lv_obj_t *parent)
{
    if (!parent) return;

    // Create alarm settings section
    s_alarm_panel = lv_obj_create(parent);
    lv_obj_set_size(s_alarm_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_alarm_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_alarm_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_alarm_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_alarm_panel, 0, 0);
    lv_obj_set_style_pad_all(s_alarm_panel, 0, 0);
    lv_obj_set_style_pad_row(s_alarm_panel, 8, 0);

    // Title
    s_alarm_title = lv_label_create(s_alarm_panel);
    lv_label_set_text(s_alarm_title, "Alarms");
    lv_obj_set_style_text_font(s_alarm_title, &lv_font_montserrat_18, 0);

    // Alarm list container
    s_alarm_list = lv_obj_create(s_alarm_panel);
    lv_obj_set_size(s_alarm_list, 230, 180);
    lv_obj_set_flex_flow(s_alarm_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_alarm_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s_alarm_list, lv_color_hex(0x252525), 0);
    lv_obj_set_style_radius(s_alarm_list, 8, 0);
    lv_obj_set_style_pad_all(s_alarm_list, 8, 0);
    lv_obj_set_style_pad_row(s_alarm_list, 6, 0);
    lv_obj_add_flag(s_alarm_list, LV_OBJ_FLAG_SCROLLABLE);

    // Add button
    s_add_btn = lv_button_create(s_alarm_panel);
    lv_obj_set_size(s_add_btn, 180, 40);
    lv_obj_add_event_cb(s_add_btn, add_alarm_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(s_add_btn, lv_color_hex(0x2196F3), 0);

    lv_obj_t *add_label = lv_label_create(s_add_btn);
    lv_label_set_text(add_label, LV_SYMBOL_PLUS " Add Alarm");
    lv_obj_set_style_text_font(add_label, &lv_font_montserrat_14, 0);
    lv_obj_center(add_label);

    // Initial update
    ui_alarm_update_list();

    ESP_LOGI(TAG, "Alarm UI initialized");
}

void ui_alarm_deinit(void)
{
    close_edit_dialog();
    ui_alarm_hide_trigger_popup();

    s_alarm_panel = NULL;
    s_alarm_title = NULL;
    s_alarm_list = NULL;
    s_add_btn = NULL;

    ESP_LOGI(TAG, "Alarm UI deinitialized");
}
