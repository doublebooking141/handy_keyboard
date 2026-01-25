/**
 * @file ui_network.c
 * @brief Network Settings UI - WiFi, NTP, and WOL controls
 */

#include "ui_network.h"
#include "network.h"
#include "ntp.h"
#include "wol.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "UI_NETWORK";

// UI elements
static lv_obj_t *s_network_panel = NULL;
static lv_obj_t *s_wifi_status_label = NULL;
static lv_obj_t *s_ntp_dropdown = NULL;
static lv_obj_t *s_sync_btn = NULL;
static lv_obj_t *s_sync_status_label = NULL;

// Manual time inputs
static lv_obj_t *s_year_dropdown = NULL;
static lv_obj_t *s_month_dropdown = NULL;
static lv_obj_t *s_day_dropdown = NULL;
static lv_obj_t *s_hour_dropdown = NULL;
static lv_obj_t *s_minute_dropdown = NULL;
static lv_obj_t *s_manual_set_btn = NULL;

// WOL elements
static lv_obj_t *s_wol_target_label = NULL;
static lv_obj_t *s_wol_btn = NULL;

// Year options (2024-2035)
static const char *YEAR_OPTIONS =
    "2024\n2025\n2026\n2027\n2028\n2029\n2030\n2031\n2032\n2033\n2034\n2035";

// Month options (01-12)
static const char *MONTH_OPTIONS =
    "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12";

// Day options (01-31)
static const char *DAY_OPTIONS =
    "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n"
    "16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31";

// Hour options (00-23)
static const char *HOUR_OPTIONS =
    "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
    "12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";

// Minute options (00-59)
static const char *MINUTE_OPTIONS =
    "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n"
    "10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
    "20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n"
    "30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n"
    "40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n"
    "50\n51\n52\n53\n54\n55\n56\n57\n58\n59";

// NTP server options
static const char *NTP_SERVER_OPTIONS = "NICT (ntp.nict.jp)\nGoogle (time.google.com)";

/**
 * @brief Update WiFi status label
 */
static void update_wifi_status(void)
{
    if (!s_wifi_status_label) return;

    char status[64];
    network_state_t state = network_get_state();

    switch (state) {
    case NETWORK_STATE_CONNECTED: {
        char ip[16];
        if (network_get_ip(ip, sizeof(ip)) == ESP_OK) {
            snprintf(status, sizeof(status), "Connected (%s)", ip);
        } else {
            snprintf(status, sizeof(status), "Connected");
        }
        lv_obj_set_style_text_color(s_wifi_status_label, lv_color_hex(0x00FF00), 0);
        break;
    }
    case NETWORK_STATE_CONNECTING:
        snprintf(status, sizeof(status), "Connecting...");
        lv_obj_set_style_text_color(s_wifi_status_label, lv_color_hex(0xFFFF00), 0);
        break;
    case NETWORK_STATE_ERROR:
        snprintf(status, sizeof(status), "Connection Failed");
        lv_obj_set_style_text_color(s_wifi_status_label, lv_color_hex(0xFF6666), 0);
        break;
    case NETWORK_STATE_DISCONNECTED:
    default:
        snprintf(status, sizeof(status), "Disconnected");
        lv_obj_set_style_text_color(s_wifi_status_label, lv_color_hex(0xAAAAAA), 0);
        break;
    }

    lv_label_set_text(s_wifi_status_label, status);
}

/**
 * @brief NTP sync button callback
 */
static void sync_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    if (!network_is_connected()) {
        if (s_sync_status_label) {
            lv_label_set_text(s_sync_status_label, "WiFi not connected!");
            lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFF6666), 0);
        }
        return;
    }

    // Get selected server
    ntp_server_t server = (ntp_server_t)lv_dropdown_get_selected(s_ntp_dropdown);

    if (s_sync_status_label) {
        lv_label_set_text(s_sync_status_label, "Syncing...");
        lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFFFF00), 0);
    }

    ESP_LOGI(TAG, "Starting NTP sync with server %d", server);

    // ntp_sync() is now async - it returns immediately after starting sync
    // Sync status will be updated via ui_network_update() polling
    esp_err_t ret = ntp_sync(server);
    if (ret != ESP_OK) {
        // Only show error if sync couldn't start (e.g., not connected)
        if (s_sync_status_label) {
            lv_label_set_text(s_sync_status_label, "Sync failed to start!");
            lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFF6666), 0);
        }
    }
    // If ret == ESP_OK, status remains "Syncing..." until update_ntp_status() updates it
}

/**
 * @brief NTP server dropdown callback
 */
static void ntp_server_changed_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    ntp_server_t server = (ntp_server_t)lv_dropdown_get_selected(s_ntp_dropdown);
    ntp_set_default_server(server);
    ESP_LOGI(TAG, "NTP server changed to: %s", ntp_get_server_name(server));
}

/**
 * @brief Manual time set button callback
 */
static void manual_set_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    struct tm time;
    memset(&time, 0, sizeof(time));

    // Get values from dropdowns
    time.tm_year = lv_dropdown_get_selected(s_year_dropdown) + 2024 - 1900;
    time.tm_mon = lv_dropdown_get_selected(s_month_dropdown);
    time.tm_mday = lv_dropdown_get_selected(s_day_dropdown) + 1;
    time.tm_hour = lv_dropdown_get_selected(s_hour_dropdown);
    time.tm_min = lv_dropdown_get_selected(s_minute_dropdown);
    time.tm_sec = 0;

    ESP_LOGI(TAG, "Setting manual time: %04d-%02d-%02d %02d:%02d",
             time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
             time.tm_hour, time.tm_min);

    esp_err_t ret = ntp_set_time_manual(&time);
    if (ret == ESP_OK) {
        if (s_sync_status_label) {
            lv_label_set_text(s_sync_status_label, "Time set!");
            lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0x00FF00), 0);
        }
    } else {
        if (s_sync_status_label) {
            lv_label_set_text(s_sync_status_label, "Failed to set time!");
            lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFF6666), 0);
        }
    }
}

/**
 * @brief WOL send button callback
 */
static void wol_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;

    wol_result_t result = wol_send_default();

    switch (result) {
    case WOL_RESULT_OK:
        ESP_LOGI(TAG, "WOL packet sent successfully");
        // Brief visual feedback on button
        lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0x00FF00), 0);
        break;
    case WOL_RESULT_NOT_CONNECTED:
        ESP_LOGW(TAG, "WOL failed: network not connected");
        lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0xFF6666), 0);
        break;
    case WOL_RESULT_INVALID_MAC:
        ESP_LOGW(TAG, "WOL failed: invalid MAC");
        lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0xFF6666), 0);
        break;
    default:
        ESP_LOGE(TAG, "WOL failed: send error");
        lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0xFF6666), 0);
        break;
    }
}

/**
 * @brief Set current time to dropdown selections
 */
static void set_current_time_to_dropdowns(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Set year (2024 = index 0)
    int year_idx = timeinfo.tm_year + 1900 - 2024;
    if (year_idx >= 0 && year_idx < 12) {
        lv_dropdown_set_selected(s_year_dropdown, year_idx);
    }

    lv_dropdown_set_selected(s_month_dropdown, timeinfo.tm_mon);
    lv_dropdown_set_selected(s_day_dropdown, timeinfo.tm_mday - 1);
    lv_dropdown_set_selected(s_hour_dropdown, timeinfo.tm_hour);
    lv_dropdown_set_selected(s_minute_dropdown, timeinfo.tm_min);
}

void ui_network_init(lv_obj_t *parent)
{
    if (!parent) return;

    ESP_LOGI(TAG, "Initializing network UI");

    // Create network settings section
    s_network_panel = lv_obj_create(parent);
    lv_obj_set_size(s_network_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_network_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_network_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_network_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_network_panel, 0, 0);
    lv_obj_set_style_pad_all(s_network_panel, 0, 0);
    lv_obj_set_style_pad_row(s_network_panel, 10, 0);

    // ========== Network & Time Title ==========
    lv_obj_t *title = lv_label_create(s_network_panel);
    lv_label_set_text(title, "Network & Time");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    // ========== WiFi Status ==========
    lv_obj_t *wifi_row = lv_obj_create(s_network_panel);
    lv_obj_set_size(wifi_row, 280, 40);
    lv_obj_set_flex_flow(wifi_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(wifi_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_row, 0, 0);
    lv_obj_set_style_pad_all(wifi_row, 0, 0);

    lv_obj_t *wifi_label = lv_label_create(wifi_row);
    lv_label_set_text(wifi_label, "WiFi:");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFFFFF), 0);

    s_wifi_status_label = lv_label_create(wifi_row);
    lv_label_set_text(s_wifi_status_label, "Checking...");
    lv_obj_set_style_text_font(s_wifi_status_label, &lv_font_montserrat_14, 0);

    // ========== NTP Server Selection ==========
    lv_obj_t *ntp_label = lv_label_create(s_network_panel);
    lv_label_set_text(ntp_label, "NTP Server:");
    lv_obj_set_style_text_font(ntp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ntp_label, lv_color_hex(0xFFFFFF), 0);

    s_ntp_dropdown = lv_dropdown_create(s_network_panel);
    lv_dropdown_set_options(s_ntp_dropdown, NTP_SERVER_OPTIONS);
    lv_dropdown_set_selected(s_ntp_dropdown, ntp_get_default_server());
    lv_obj_set_width(s_ntp_dropdown, 250);
    lv_obj_set_style_text_font(s_ntp_dropdown, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(s_ntp_dropdown, ntp_server_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Sync button row
    lv_obj_t *sync_row = lv_obj_create(s_network_panel);
    lv_obj_set_size(sync_row, 280, 50);
    lv_obj_set_flex_flow(sync_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sync_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(sync_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sync_row, 0, 0);
    lv_obj_set_style_pad_all(sync_row, 0, 0);

    s_sync_btn = lv_button_create(sync_row);
    lv_obj_set_size(s_sync_btn, 120, 40);
    lv_obj_add_event_cb(s_sync_btn, sync_btn_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(s_sync_btn, lv_color_hex(0x2196F3), 0);

    lv_obj_t *sync_btn_label = lv_label_create(s_sync_btn);
    lv_label_set_text(sync_btn_label, "Sync Now");
    lv_obj_set_style_text_font(sync_btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(sync_btn_label);

    s_sync_status_label = lv_label_create(sync_row);
    lv_label_set_text(s_sync_status_label, "");
    lv_obj_set_style_text_font(s_sync_status_label, &lv_font_montserrat_12, 0);

    // ========== Manual Time Section ==========
    lv_obj_t *manual_label = lv_label_create(s_network_panel);
    lv_label_set_text(manual_label, "Manual Time:");
    lv_obj_set_style_text_font(manual_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(manual_label, lv_color_hex(0xFFFFFF), 0);

    // Date row: Year Month Day
    lv_obj_t *date_row = lv_obj_create(s_network_panel);
    lv_obj_set_size(date_row, 280, 50);
    lv_obj_set_flex_flow(date_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(date_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(date_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(date_row, 0, 0);
    lv_obj_set_style_pad_all(date_row, 0, 0);
    lv_obj_set_style_pad_column(date_row, 8, 0);

    s_year_dropdown = lv_dropdown_create(date_row);
    lv_dropdown_set_options(s_year_dropdown, YEAR_OPTIONS);
    lv_obj_set_width(s_year_dropdown, 85);
    lv_obj_set_style_text_font(s_year_dropdown, &lv_font_montserrat_14, 0);

    s_month_dropdown = lv_dropdown_create(date_row);
    lv_dropdown_set_options(s_month_dropdown, MONTH_OPTIONS);
    lv_obj_set_width(s_month_dropdown, 65);
    lv_obj_set_style_text_font(s_month_dropdown, &lv_font_montserrat_14, 0);

    s_day_dropdown = lv_dropdown_create(date_row);
    lv_dropdown_set_options(s_day_dropdown, DAY_OPTIONS);
    lv_obj_set_width(s_day_dropdown, 65);
    lv_obj_set_style_text_font(s_day_dropdown, &lv_font_montserrat_14, 0);

    // Time row: Hour : Minute
    lv_obj_t *time_row = lv_obj_create(s_network_panel);
    lv_obj_set_size(time_row, 280, 50);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_row, 0, 0);
    lv_obj_set_style_pad_all(time_row, 0, 0);
    lv_obj_set_style_pad_column(time_row, 8, 0);

    s_hour_dropdown = lv_dropdown_create(time_row);
    lv_dropdown_set_options(s_hour_dropdown, HOUR_OPTIONS);
    lv_obj_set_width(s_hour_dropdown, 80);
    lv_obj_set_style_text_font(s_hour_dropdown, &lv_font_montserrat_14, 0);

    lv_obj_t *colon = lv_label_create(time_row);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(colon, lv_color_hex(0xFFFFFF), 0);

    s_minute_dropdown = lv_dropdown_create(time_row);
    lv_dropdown_set_options(s_minute_dropdown, MINUTE_OPTIONS);
    lv_obj_set_width(s_minute_dropdown, 80);
    lv_obj_set_style_text_font(s_minute_dropdown, &lv_font_montserrat_14, 0);

    // Set current time to dropdowns
    set_current_time_to_dropdowns();

    // Set Manual button
    s_manual_set_btn = lv_button_create(s_network_panel);
    lv_obj_set_size(s_manual_set_btn, 140, 40);
    lv_obj_add_event_cb(s_manual_set_btn, manual_set_btn_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(s_manual_set_btn, lv_color_hex(0x757575), 0);

    lv_obj_t *manual_btn_label = lv_label_create(s_manual_set_btn);
    lv_label_set_text(manual_btn_label, "Set Manual");
    lv_obj_set_style_text_font(manual_btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(manual_btn_label);

    // ========== WOL Section ==========
    lv_obj_t *wol_separator = lv_obj_create(s_network_panel);
    lv_obj_set_size(wol_separator, 200, 2);
    lv_obj_set_style_bg_color(wol_separator, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(wol_separator, 0, 0);
    lv_obj_set_style_pad_all(wol_separator, 0, 0);

    lv_obj_t *wol_title = lv_label_create(s_network_panel);
    lv_label_set_text(wol_title, "Wake-on-LAN");
    lv_obj_set_style_text_font(wol_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(wol_title, lv_color_hex(0xFFFFFF), 0);

    // Target MAC display
    char mac_str[20] = "Not configured";
    wol_get_default_mac_str(mac_str, sizeof(mac_str));

    lv_obj_t *target_row = lv_obj_create(s_network_panel);
    lv_obj_set_size(target_row, 280, 30);
    lv_obj_set_flex_flow(target_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(target_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(target_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(target_row, 0, 0);
    lv_obj_set_style_pad_all(target_row, 0, 0);
    lv_obj_set_style_pad_column(target_row, 8, 0);

    lv_obj_t *target_label = lv_label_create(target_row);
    lv_label_set_text(target_label, "Target:");
    lv_obj_set_style_text_font(target_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(target_label, lv_color_hex(0xFFFFFF), 0);

    s_wol_target_label = lv_label_create(target_row);
    lv_label_set_text(s_wol_target_label, mac_str);
    lv_obj_set_style_text_font(s_wol_target_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_wol_target_label, lv_color_hex(0xAAAAAA), 0);

    // Send WOL button
    s_wol_btn = lv_button_create(s_network_panel);
    lv_obj_set_size(s_wol_btn, 140, 40);
    lv_obj_add_event_cb(s_wol_btn, wol_btn_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0xFF9800), 0);

    lv_obj_t *wol_btn_label = lv_label_create(s_wol_btn);
    lv_label_set_text(wol_btn_label, "Send WOL");
    lv_obj_set_style_text_font(wol_btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(wol_btn_label);

    // Initial status update
    update_wifi_status();

    ESP_LOGI(TAG, "Network UI initialized");
}

void ui_network_deinit(void)
{
    s_network_panel = NULL;
    s_wifi_status_label = NULL;
    s_ntp_dropdown = NULL;
    s_sync_btn = NULL;
    s_sync_status_label = NULL;
    s_year_dropdown = NULL;
    s_month_dropdown = NULL;
    s_day_dropdown = NULL;
    s_hour_dropdown = NULL;
    s_minute_dropdown = NULL;
    s_manual_set_btn = NULL;
    s_wol_target_label = NULL;
    s_wol_btn = NULL;

    ESP_LOGI(TAG, "Network UI deinitialized");
}

void ui_network_update(void)
{
    update_wifi_status();

    // Update NTP sync status label based on current NTP state
    if (s_sync_status_label) {
        ntp_status_t ntp_status = ntp_get_status();
        static ntp_status_t last_ntp_status = NTP_STATUS_IDLE;

        if (ntp_status != last_ntp_status) {
            last_ntp_status = ntp_status;

            switch (ntp_status) {
            case NTP_STATUS_SYNCED:
                lv_label_set_text(s_sync_status_label, "Synced!");
                lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0x00FF00), 0);
                break;
            case NTP_STATUS_SYNCING:
                lv_label_set_text(s_sync_status_label, "Syncing...");
                lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFFFF00), 0);
                break;
            case NTP_STATUS_FAILED:
                lv_label_set_text(s_sync_status_label, "Sync failed!");
                lv_obj_set_style_text_color(s_sync_status_label, lv_color_hex(0xFF6666), 0);
                break;
            case NTP_STATUS_IDLE:
            default:
                // Keep current text
                break;
            }
        }
    }

    // Reset WOL button color after press
    if (s_wol_btn) {
        lv_obj_set_style_bg_color(s_wol_btn, lv_color_hex(0xFF9800), 0);
    }
}
