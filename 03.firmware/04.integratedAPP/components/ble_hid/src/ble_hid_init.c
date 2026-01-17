/**
 * @file ble_hid_init.c
 * @brief BLE HID Core Implementation (NimBLE over ESP-Hosted)
 *
 * Based on keyboard_2025 reference implementation.
 * ESP-Hosted経由でESP32-C6のNimBLE機能を使用。
 */

#include "ble_hid.h"
#include "ble_hid_keycodes.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_hosted.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string.h>
#include <inttypes.h>

// Forward declaration for NimBLE store config initialization
void ble_store_config_init(void);

static const char *TAG = "BLE_HID";

// ============================================================================
// Configuration
// ============================================================================

#define DEVICE_NAME             "Handy Keyboard"
#define DEVICE_MANUFACTURER     "doublebooking"
#define PNP_VENDOR_ID           0x02E5  // Espressif
#define PNP_PRODUCT_ID          0x4001
#define PNP_VERSION             0x0100

#define REPORT_ID_KEYBOARD      1
#define REPORT_ID_MOUSE         2
#define REPORT_ID_CONSUMER      3

#define ADV_INTERVAL_MIN_MS     30
#define ADV_INTERVAL_MAX_MS     60

// ============================================================================
// ASCII to HID Keycode Mapping
// ============================================================================

typedef struct {
    char character;
    uint8_t keycode;
    bool shift;
} ascii_map_entry_t;

static const ascii_map_entry_t ascii_map[] = {
    // Letters (lowercase)
    {'a', HID_KEY_A, false}, {'b', HID_KEY_B, false}, {'c', HID_KEY_C, false},
    {'d', HID_KEY_D, false}, {'e', HID_KEY_E, false}, {'f', HID_KEY_F, false},
    {'g', HID_KEY_G, false}, {'h', HID_KEY_H, false}, {'i', HID_KEY_I, false},
    {'j', HID_KEY_J, false}, {'k', HID_KEY_K, false}, {'l', HID_KEY_L, false},
    {'m', HID_KEY_M, false}, {'n', HID_KEY_N, false}, {'o', HID_KEY_O, false},
    {'p', HID_KEY_P, false}, {'q', HID_KEY_Q, false}, {'r', HID_KEY_R, false},
    {'s', HID_KEY_S, false}, {'t', HID_KEY_T, false}, {'u', HID_KEY_U, false},
    {'v', HID_KEY_V, false}, {'w', HID_KEY_W, false}, {'x', HID_KEY_X, false},
    {'y', HID_KEY_Y, false}, {'z', HID_KEY_Z, false},
    // Letters (uppercase)
    {'A', HID_KEY_A, true}, {'B', HID_KEY_B, true}, {'C', HID_KEY_C, true},
    {'D', HID_KEY_D, true}, {'E', HID_KEY_E, true}, {'F', HID_KEY_F, true},
    {'G', HID_KEY_G, true}, {'H', HID_KEY_H, true}, {'I', HID_KEY_I, true},
    {'J', HID_KEY_J, true}, {'K', HID_KEY_K, true}, {'L', HID_KEY_L, true},
    {'M', HID_KEY_M, true}, {'N', HID_KEY_N, true}, {'O', HID_KEY_O, true},
    {'P', HID_KEY_P, true}, {'Q', HID_KEY_Q, true}, {'R', HID_KEY_R, true},
    {'S', HID_KEY_S, true}, {'T', HID_KEY_T, true}, {'U', HID_KEY_U, true},
    {'V', HID_KEY_V, true}, {'W', HID_KEY_W, true}, {'X', HID_KEY_X, true},
    {'Y', HID_KEY_Y, true}, {'Z', HID_KEY_Z, true},
    // Numbers
    {'1', HID_KEY_1, false}, {'2', HID_KEY_2, false}, {'3', HID_KEY_3, false},
    {'4', HID_KEY_4, false}, {'5', HID_KEY_5, false}, {'6', HID_KEY_6, false},
    {'7', HID_KEY_7, false}, {'8', HID_KEY_8, false}, {'9', HID_KEY_9, false},
    {'0', HID_KEY_0, false},
    // Symbols (no shift)
    {' ', HID_KEY_SPACE, false},
    {'-', HID_KEY_MINUS, false},
    {'=', HID_KEY_EQUAL, false},
    {'[', HID_KEY_LBRACKET, false},
    {']', HID_KEY_RBRACKET, false},
    {'\\', HID_KEY_BACKSLASH, false},
    {';', HID_KEY_SEMICOLON, false},
    {'\'', HID_KEY_APOSTROPHE, false},
    {'`', HID_KEY_GRAVE, false},
    {',', HID_KEY_COMMA, false},
    {'.', HID_KEY_DOT, false},
    {'/', HID_KEY_SLASH, false},
    // Symbols (with shift)
    {'!', HID_KEY_1, true},
    {'@', HID_KEY_2, true},
    {'#', HID_KEY_3, true},
    {'$', HID_KEY_4, true},
    {'%', HID_KEY_5, true},
    {'^', HID_KEY_6, true},
    {'&', HID_KEY_7, true},
    {'*', HID_KEY_8, true},
    {'(', HID_KEY_9, true},
    {')', HID_KEY_0, true},
    {'_', HID_KEY_MINUS, true},
    {'+', HID_KEY_EQUAL, true},
    {'{', HID_KEY_LBRACKET, true},
    {'}', HID_KEY_RBRACKET, true},
    {'|', HID_KEY_BACKSLASH, true},
    {':', HID_KEY_SEMICOLON, true},
    {'"', HID_KEY_APOSTROPHE, true},
    {'~', HID_KEY_GRAVE, true},
    {'<', HID_KEY_COMMA, true},
    {'>', HID_KEY_DOT, true},
    {'?', HID_KEY_SLASH, true},
    // Control characters
    {'\n', HID_KEY_ENTER, false},
    {'\r', HID_KEY_ENTER, false},
    {'\t', HID_KEY_TAB, false},
};

#define ASCII_MAP_SIZE (sizeof(ascii_map) / sizeof(ascii_map[0]))

bool ascii_to_hid_keycode(char c, uint8_t *keycode, bool *shift)
{
    for (size_t i = 0; i < ASCII_MAP_SIZE; i++) {
        if (ascii_map[i].character == c) {
            *keycode = ascii_map[i].keycode;
            *shift = ascii_map[i].shift;
            return true;
        }
    }
    return false;
}

// ============================================================================
// HID Report Descriptor
// ============================================================================

static const uint8_t hid_report_descriptor[] = {
    // Keyboard Report (Report ID = 1)
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,

    // Modifier keys (8 bits)
    0x05, 0x07,        // Usage Page (Key Codes)
    0x19, 0xE0,        // Usage Minimum (224)
    0x29, 0xE7,        // Usage Maximum (231)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x01,        // Logical Maximum (1)
    0x75, 0x01,        // Report Size (1 bit)
    0x95, 0x08,        // Report Count (8)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    // Reserved byte
    0x95, 0x01,        // Report Count (1)
    0x75, 0x08,        // Report Size (8 bits)
    0x81, 0x01,        // Input (Constant)

    // LED Output Report
    0x05, 0x08,        // Usage Page (LEDs)
    0x19, 0x01,        // Usage Minimum (1)
    0x29, 0x05,        // Usage Maximum (5)
    0x95, 0x05,        // Report Count (5)
    0x75, 0x01,        // Report Size (1 bit)
    0x91, 0x02,        // Output (Data, Variable, Absolute)
    0x95, 0x01,        // Report Count (1)
    0x75, 0x03,        // Report Size (3 bits)
    0x91, 0x01,        // Output (Constant)

    // 6KRO keys
    0x05, 0x07,        // Usage Page (Key Codes)
    0x19, 0x00,        // Usage Minimum (0)
    0x29, 0x65,        // Usage Maximum (101)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x65,        // Logical Maximum (101)
    0x95, 0x06,        // Report Count (6)
    0x75, 0x08,        // Report Size (8 bits)
    0x81, 0x00,        // Input (Data, Array)

    0xC0,              // End Collection

    // =========================================================================
    // Mouse Report (Report ID = 2)
    // =========================================================================
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_MOUSE,  // Report ID (2)
    0x09, 0x01,        // Usage (Pointer)
    0xA1, 0x00,        // Collection (Physical)

    // Buttons (3 bits: Left, Right, Middle)
    0x05, 0x09,        // Usage Page (Buttons)
    0x19, 0x01,        // Usage Minimum (1 - Button 1)
    0x29, 0x03,        // Usage Maximum (3 - Button 3)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x01,        // Logical Maximum (1)
    0x95, 0x03,        // Report Count (3)
    0x75, 0x01,        // Report Size (1 bit)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    // Padding (5 bits)
    0x95, 0x01,        // Report Count (1)
    0x75, 0x05,        // Report Size (5 bits)
    0x81, 0x01,        // Input (Constant)

    // X, Y movement (relative)
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x30,        // Usage (X)
    0x09, 0x31,        // Usage (Y)
    0x15, 0x81,        // Logical Minimum (-127)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x02,        // Report Count (2)
    0x81, 0x06,        // Input (Data, Variable, Relative)

    // Wheel (vertical scroll)
    0x09, 0x38,        // Usage (Wheel)
    0x15, 0x81,        // Logical Minimum (-127)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x06,        // Input (Data, Variable, Relative)

    // Horizontal Scroll (AC Pan)
    0x05, 0x0C,        // Usage Page (Consumer)
    0x0A, 0x38, 0x02,  // Usage (AC Pan)
    0x15, 0x81,        // Logical Minimum (-127)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x06,        // Input (Data, Variable, Relative)

    0xC0,              // End Collection (Physical)
    0xC0,              // End Collection (Application)

    // =========================================================================
    // Consumer Control Report (Report ID = 3)
    // =========================================================================
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_CONSUMER,  // Report ID (3)
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0xFF, 0x03,  // Logical Maximum (1023)
    0x19, 0x00,        // Usage Minimum (0)
    0x2A, 0xFF, 0x03,  // Usage Maximum (1023)
    0x75, 0x10,        // Report Size (16 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x00,        // Input (Data, Array)
    0xC0               // End Collection
};

// ============================================================================
// State Variables
// ============================================================================

static bool g_initialized = false;
static bool g_connected = false;
static bool g_keyboard_subscribed = false;  // Track notification subscription
static bool g_mouse_subscribed = false;
static bool g_consumer_subscribed = false;
static bool g_auto_reconnect = true;         // Auto-reconnect on disconnect
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static SemaphoreHandle_t g_send_mutex = NULL;
static uint8_t g_own_addr_type;
static ble_hid_event_cb_t g_event_callback = NULL;

// GATT characteristic handles
static uint16_t g_keyboard_handle = 0;
static uint16_t g_mouse_handle = 0;
static uint16_t g_consumer_handle = 0;

// ============================================================================
// GATT UUIDs
// ============================================================================

static const ble_uuid16_t hid_svc_uuid = BLE_UUID16_INIT(0x1812);
static const ble_uuid16_t hid_info_uuid = BLE_UUID16_INIT(0x2A4A);
static const ble_uuid16_t hid_report_map_uuid = BLE_UUID16_INIT(0x2A4B);
static const ble_uuid16_t hid_ctrl_point_uuid = BLE_UUID16_INIT(0x2A4C);
static const ble_uuid16_t hid_report_uuid = BLE_UUID16_INIT(0x2A4D);
static const ble_uuid16_t hid_protocol_mode_uuid = BLE_UUID16_INIT(0x2A4E);
static const ble_uuid16_t dis_svc_uuid = BLE_UUID16_INIT(0x180A);
static const ble_uuid16_t pnp_id_uuid = BLE_UUID16_INIT(0x2A50);
static const ble_uuid16_t manufacturer_name_uuid = BLE_UUID16_INIT(0x2A29);
static const ble_uuid16_t report_ref_uuid = BLE_UUID16_INIT(0x2908);
static const ble_uuid16_t bas_svc_uuid = BLE_UUID16_INIT(0x180F);
static const ble_uuid16_t battery_level_uuid = BLE_UUID16_INIT(0x2A19);

// ============================================================================
// HID Info / Protocol State
// ============================================================================

static const uint8_t hid_info_value[] = { 0x11, 0x01, 0x00, 0x02 };
static uint8_t g_protocol_mode = 1;
static uint8_t g_control_point = 0;

static const uint8_t pnp_id_value[] = {
    0x02,
    (PNP_VENDOR_ID & 0xFF), (PNP_VENDOR_ID >> 8) & 0xFF,
    (PNP_PRODUCT_ID & 0xFF), (PNP_PRODUCT_ID >> 8) & 0xFF,
    (PNP_VERSION & 0xFF), (PNP_VERSION >> 8) & 0xFF
};

static uint8_t keyboard_input_ref[] = { REPORT_ID_KEYBOARD, 0x01 };
static uint8_t keyboard_output_ref[] = { REPORT_ID_KEYBOARD, 0x02 };
static uint8_t mouse_input_ref[] = { REPORT_ID_MOUSE, 0x01 };
static uint8_t consumer_input_ref[] = { REPORT_ID_CONSUMER, 0x01 };

static uint8_t g_battery_level = 100;

// ============================================================================
// Forward Declarations
// ============================================================================

static int ble_hid_gap_event(struct ble_gap_event *event, void *arg);
static void ble_hid_on_sync(void);
static void ble_hid_on_reset(int reason);

// ============================================================================
// GATT Access Callbacks
// ============================================================================

static int gatt_hid_info_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, hid_info_value, sizeof(hid_info_value));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_hid_report_map_access(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, hid_report_descriptor, sizeof(hid_report_descriptor));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_hid_ctrl_point_access(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len == 1) {
            ble_hs_mbuf_to_flat(ctxt->om, &g_control_point, 1, NULL);
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_hid_protocol_mode_access(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &g_protocol_mode, sizeof(g_protocol_mode));
        return 0;
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len == 1) {
            ble_hs_mbuf_to_flat(ctxt->om, &g_protocol_mode, 1, NULL);
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_hid_report_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uintptr_t report_id = (uintptr_t)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t empty[8] = {0};
        int size = 2;  // Default for consumer
        if (report_id == REPORT_ID_KEYBOARD) {
            size = 8;  // modifier + reserved + 6 keys
        } else if (report_id == REPORT_ID_MOUSE) {
            size = 5;  // buttons + X + Y + wheel + pan
        }
        os_mbuf_append(ctxt->om, empty, size);
        return 0;
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        // LED output report (keyboard only)
        if (report_id == REPORT_ID_KEYBOARD) {
            uint8_t led_state;
            ble_hs_mbuf_to_flat(ctxt->om, &led_state, 1, NULL);
            ESP_LOGD(TAG, "LED state: 0x%02X", led_state);
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_hid_report_ref_access(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t *ref = (uint8_t *)arg;
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        os_mbuf_append(ctxt->om, ref, 2);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_dis_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid = ctxt->chr->uuid;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (ble_uuid_cmp(uuid, &manufacturer_name_uuid.u) == 0) {
            os_mbuf_append(ctxt->om, DEVICE_MANUFACTURER, strlen(DEVICE_MANUFACTURER));
        } else if (ble_uuid_cmp(uuid, &pnp_id_uuid.u) == 0) {
            os_mbuf_append(ctxt->om, pnp_id_value, sizeof(pnp_id_value));
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_bas_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &g_battery_level, sizeof(g_battery_level));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// ============================================================================
// GATT Service Definition
// ============================================================================

static const struct ble_gatt_svc_def gatt_svcs[] = {
    // HID Service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &hid_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            // HID Information
            {
                .uuid = &hid_info_uuid.u,
                .access_cb = gatt_hid_info_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            // HID Report Map
            {
                .uuid = &hid_report_map_uuid.u,
                .access_cb = gatt_hid_report_map_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            // HID Control Point
            {
                .uuid = &hid_ctrl_point_uuid.u,
                .access_cb = gatt_hid_ctrl_point_access,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            // HID Protocol Mode
            {
                .uuid = &hid_protocol_mode_uuid.u,
                .access_cb = gatt_hid_protocol_mode_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            // Keyboard Input Report
            {
                .uuid = &hid_report_uuid.u,
                .access_cb = gatt_hid_report_access,
                .arg = (void *)(uintptr_t)REPORT_ID_KEYBOARD,
                .val_handle = &g_keyboard_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = &report_ref_uuid.u,
                        .access_cb = gatt_hid_report_ref_access,
                        .arg = keyboard_input_ref,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    { 0 }
                },
            },
            // Keyboard Output Report (LED)
            {
                .uuid = &hid_report_uuid.u,
                .access_cb = gatt_hid_report_access,
                .arg = (void *)(uintptr_t)REPORT_ID_KEYBOARD,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = &report_ref_uuid.u,
                        .access_cb = gatt_hid_report_ref_access,
                        .arg = keyboard_output_ref,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    { 0 }
                },
            },
            // Mouse Input Report
            {
                .uuid = &hid_report_uuid.u,
                .access_cb = gatt_hid_report_access,
                .arg = (void *)(uintptr_t)REPORT_ID_MOUSE,
                .val_handle = &g_mouse_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = &report_ref_uuid.u,
                        .access_cb = gatt_hid_report_ref_access,
                        .arg = mouse_input_ref,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    { 0 }
                },
            },
            // Consumer Input Report
            {
                .uuid = &hid_report_uuid.u,
                .access_cb = gatt_hid_report_access,
                .arg = (void *)(uintptr_t)REPORT_ID_CONSUMER,
                .val_handle = &g_consumer_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = &report_ref_uuid.u,
                        .access_cb = gatt_hid_report_ref_access,
                        .arg = consumer_input_ref,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    { 0 }
                },
            },
            { 0 }
        },
    },
    // Device Information Service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &dis_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &manufacturer_name_uuid.u,
                .access_cb = gatt_dis_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = &pnp_id_uuid.u,
                .access_cb = gatt_dis_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 }
        },
    },
    // Battery Service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &bas_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &battery_level_uuid.u,
                .access_cb = gatt_bas_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 }
        },
    },
    { 0 }
};

// ============================================================================
// Advertising
// ============================================================================

static int start_advertising(void)
{
    // Don't restart if already advertising
    if (ble_gap_adv_active()) {
        return 0;
    }

    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;
    fields.appearance = 0x03C1;  // Keyboard
    fields.appearance_is_present = 1;

    // Include HID Service UUID
    fields.uuids16 = (ble_uuid16_t[]){ BLE_UUID16_INIT(0x1812) };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Set adv fields failed: %d", rc);
        return rc;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = ADV_INTERVAL_MIN_MS * 1000 / 625;
    adv_params.itvl_max = ADV_INTERVAL_MAX_MS * 1000 / 625;

    rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_hid_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Start advertising failed: %d", rc);
        return rc;
    }

    ESP_LOGI(TAG, "Advertising started as '%s'", DEVICE_NAME);
    return 0;
}

// ============================================================================
// GAP Event Handler
// ============================================================================

static int ble_hid_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "GAP CONNECT event: status=%d, handle=%d",
                 event->connect.status, event->connect.conn_handle);

        // Check if connection handle is valid (even if status != 0)
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);

        if (event->connect.status == 0) {
            // Normal successful connection
            g_conn_handle = event->connect.conn_handle;
            g_connected = true;
            g_keyboard_subscribed = false;
            g_mouse_subscribed = false;
            g_consumer_subscribed = false;

            if (rc == 0) {
                ESP_LOGI(TAG, "Connected! handle=%" PRIu16 " peer=%02X:%02X:%02X:%02X:%02X:%02X",
                         g_conn_handle,
                         desc.peer_id_addr.val[5], desc.peer_id_addr.val[4],
                         desc.peer_id_addr.val[3], desc.peer_id_addr.val[2],
                         desc.peer_id_addr.val[1], desc.peer_id_addr.val[0]);
            }

            // Notify callback
            if (g_event_callback) {
                g_event_callback(BLE_HID_STATE_CONNECTED);
            }
        } else {
            // ESP-Hosted quirk: may report non-zero status even when connection is valid
            // Check if connection handle is actually valid and accept it anyway
            if (rc == 0) {
                ESP_LOGW(TAG, "Connection status=%d but handle %d is valid, accepting connection (ESP-Hosted quirk)",
                         event->connect.status, event->connect.conn_handle);
                g_conn_handle = event->connect.conn_handle;
                g_connected = true;
                g_keyboard_subscribed = false;
                g_mouse_subscribed = false;
                g_consumer_subscribed = false;

                ESP_LOGI(TAG, "Connected (quirk)! peer=%02X:%02X:%02X:%02X:%02X:%02X",
                         desc.peer_id_addr.val[5], desc.peer_id_addr.val[4],
                         desc.peer_id_addr.val[3], desc.peer_id_addr.val[2],
                         desc.peer_id_addr.val[1], desc.peer_id_addr.val[0]);

                // Notify callback
                if (g_event_callback) {
                    g_event_callback(BLE_HID_STATE_CONNECTED);
                }
            } else {
                // Decode common HCI status codes
                const char *err_str = "unknown";
                switch (event->connect.status) {
                    case 0x06: err_str = "Pin/Key Missing"; break;
                    case 0x13: err_str = "Remote User Terminated"; break;
                    case 0x16: err_str = "Local Host Terminated"; break;
                    case 0x1A: err_str = "Unsupported Remote Feature"; break;
                    case 0x22: err_str = "Instant Passed"; break;
                    case 0x28: err_str = "Pairing Not Supported"; break;
                }
                ESP_LOGW(TAG, "Connection failed: status=%d (0x%02X: %s)",
                         event->connect.status, event->connect.status, err_str);
                start_advertising();
            }
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnected");
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_connected = false;
        g_keyboard_subscribed = false;
        g_mouse_subscribed = false;
        g_consumer_subscribed = false;

        // Notify callback
        if (g_event_callback) {
            g_event_callback(BLE_HID_STATE_DISCONNECTED);
        }

        // Auto-reconnect: restart advertising
        if (g_auto_reconnect) {
            start_advertising();
        }
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        if (!g_connected) {
            start_advertising();
        }
        break;

    case BLE_GAP_EVENT_ENC_CHANGE:
        ESP_LOGI(TAG, "Encryption change: status=%d, conn_handle=%d",
                 event->enc_change.status, event->enc_change.conn_handle);
        // Encryption enabled means bonding completed (for Just Works pairing)
        if (event->enc_change.status == 0) {
            ESP_LOGI(TAG, "Bonding completed (encryption enabled)");
            // Notify UI to refresh bonded device list
            if (g_event_callback) {
                g_event_callback(BLE_HID_STATE_CONNECTED);
            }
        }
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        // Track notification subscription
        if (event->subscribe.attr_handle == g_keyboard_handle) {
            g_keyboard_subscribed = event->subscribe.cur_notify;
        } else if (event->subscribe.attr_handle == g_mouse_handle) {
            g_mouse_subscribed = event->subscribe.cur_notify;
        } else if (event->subscribe.attr_handle == g_consumer_handle) {
            g_consumer_subscribed = event->subscribe.cur_notify;
        }
        break;

    case BLE_GAP_EVENT_NOTIFY_TX:
        // Notification sent - no action needed
        break;

    case BLE_GAP_EVENT_MTU:
        // MTU updated - no action needed
        break;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        if (rc == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        // Just Works - no action needed
        break;

    default:
        break;
    }
    return 0;
}

// ============================================================================
// NimBLE Callbacks
// ============================================================================

static void ble_hid_on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE reset: %d", reason);
}

static void ble_hid_on_sync(void)
{
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to ensure address: %d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to infer address type: %d", rc);
        return;
    }

    uint8_t addr[6];
    ble_hs_id_copy_addr(g_own_addr_type, addr, NULL);
    ESP_LOGI(TAG, "BLE Address: %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    // Check if there are bonded devices
    ble_addr_t peer_addrs[CONFIG_BT_NIMBLE_MAX_BONDS];
    int num_peers = 0;
    rc = ble_store_util_bonded_peers(peer_addrs, &num_peers, CONFIG_BT_NIMBLE_MAX_BONDS);

    if (rc == 0 && num_peers > 0) {
        // Have bonded devices - start advertising for reconnection
        ESP_LOGI(TAG, "Found %d bonded device(s), starting advertising", num_peers);
        start_advertising();
    } else {
        // No bonded devices - wait for user to initiate from Settings
        ESP_LOGI(TAG, "No bonded devices, waiting for pairing from Settings");
    }
}

static void ble_hid_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t ble_hid_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    esp_err_t ret;
    int rc;

    ESP_LOGI(TAG, "Initializing BLE HID...");

    // Create mutex
    g_send_mutex = xSemaphoreCreateMutex();
    if (!g_send_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    // Step 1: Connect to ESP-Hosted coprocessor
    ESP_LOGI(TAG, "Connecting to ESP-Hosted coprocessor...");
    ret = esp_hosted_connect_to_slave();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-Hosted connect failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    ESP_LOGI(TAG, "ESP-Hosted connected!");

    // Get coprocessor FW version
    esp_hosted_coprocessor_fwver_t fwver;
    if (esp_hosted_get_coprocessor_fwversion(&fwver) == ESP_OK) {
        ESP_LOGI(TAG, "Coprocessor FW: %lu.%lu.%lu",
                 fwver.major1, fwver.minor1, fwver.patch1);
    }

    // Step 2: Initialize BT controller
    ret = esp_hosted_bt_controller_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = esp_hosted_bt_controller_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Step 3: Initialize NimBLE
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE init failed: %d", ret);
        goto cleanup;
    }

    // Step 4: Configure NimBLE host
    ble_hs_cfg.sync_cb = ble_hid_on_sync;
    ble_hs_cfg.reset_cb = ble_hid_on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Security: Match keyboard_2025 (Secure Connections enabled)
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;  // No I/O capability (Just Works)
    ble_hs_cfg.sm_bonding = 1;                    // Enable bonding
    ble_hs_cfg.sm_mitm = 0;                       // No MITM protection (Just Works)
    ble_hs_cfg.sm_sc = 1;                         // Enable Secure Connections
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    // Step 5: Initialize GAP and GATT
    ble_svc_gap_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_svc_gap_device_appearance_set(0x03C1);  // Keyboard
    ble_svc_gatt_init();

    // Step 6: Register HID GATT services
    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT count failed: %d", rc);
        goto cleanup;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT add services failed: %d", rc);
        goto cleanup;
    }

    // Step 7: Initialize bond storage
    ble_store_config_init();

    // Step 8: Start NimBLE host task
    nimble_port_freertos_init(ble_hid_host_task);

    g_initialized = true;
    ESP_LOGI(TAG, "BLE HID initialized successfully");
    return ESP_OK;

cleanup:
    if (g_send_mutex) {
        vSemaphoreDelete(g_send_mutex);
        g_send_mutex = NULL;
    }
    return ESP_FAIL;
}

esp_err_t ble_hid_deinit(void)
{
    if (!g_initialized) {
        return ESP_OK;
    }

    nimble_port_stop();
    nimble_port_deinit();

    if (g_send_mutex) {
        vSemaphoreDelete(g_send_mutex);
        g_send_mutex = NULL;
    }

    g_initialized = false;
    g_connected = false;
    return ESP_OK;
}

ble_hid_state_t ble_hid_get_state(void)
{
    if (!g_initialized) return BLE_HID_STATE_IDLE;
    if (g_connected) return BLE_HID_STATE_CONNECTED;
    if (ble_gap_adv_active()) return BLE_HID_STATE_ADVERTISING;
    return BLE_HID_STATE_DISCONNECTED;
}

bool ble_hid_is_connected(void)
{
    return g_connected;
}

void ble_hid_register_event_callback(ble_hid_event_cb_t cb)
{
    g_event_callback = cb;
    ESP_LOGI(TAG, "Event callback registered: %p", (void*)cb);
}

esp_err_t ble_hid_start_advertising(void)
{
    if (g_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    return (start_advertising() == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t ble_hid_stop_advertising(void)
{
    int rc = ble_gap_adv_stop();
    return (rc == 0) ? ESP_OK : ESP_FAIL;
}

// ============================================================================
// HID Report Sending
// ============================================================================

esp_err_t ble_hid_send_keyboard(uint8_t modifiers, const uint8_t *keys, size_t key_count)
{
    if (!g_initialized) {
        ESP_LOGW(TAG, "Send failed: not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!g_connected) {
        ESP_LOGW(TAG, "Send failed: not connected");
        return ESP_ERR_INVALID_STATE;
    }
    if (g_keyboard_handle == 0) {
        ESP_LOGW(TAG, "Send failed: keyboard handle is 0");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t report[8] = {0};
    report[0] = modifiers;
    report[1] = 0;  // Reserved

    size_t count = (key_count > 6) ? 6 : key_count;
    if (keys) {
        for (size_t i = 0; i < count; i++) {
            report[2 + i] = keys[i];
        }
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    if (!om) {
        ESP_LOGE(TAG, "Send failed: no memory for mbuf");
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_keyboard_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notify failed: rc=%d, conn=%" PRIu16 ", handle=%" PRIu16,
                 rc, g_conn_handle, g_keyboard_handle);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent key: mod=0x%02X key=0x%02X", modifiers, keys ? keys[0] : 0);
    return ESP_OK;
}

esp_err_t ble_hid_send_key(uint8_t modifiers, uint8_t keycode)
{
    esp_err_t ret;
    uint8_t keys[1] = {keycode};

    ret = ble_hid_send_keyboard(modifiers, keys, 1);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(10));

    ret = ble_hid_send_keyboard(0, NULL, 0);
    return ret;
}

esp_err_t ble_hid_send_string(const char *str)
{
    if (!str) return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Sending string: %s", str);
    esp_err_t ret = ESP_OK;

    for (const char *p = str; *p && ret == ESP_OK; p++) {
        uint8_t keycode;
        bool shift;

        if (ascii_to_hid_keycode(*p, &keycode, &shift)) {
            uint8_t modifiers = shift ? HID_MOD_LSHIFT : 0;
            ret = ble_hid_send_key(modifiers, keycode);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    return ret;
}

esp_err_t ble_hid_send_consumer(uint16_t usage)
{
    if (!g_connected || g_consumer_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t report[2] = {
        usage & 0xFF,
        (usage >> 8) & 0xFF
    };

    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    if (!om) {
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_consumer_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notify consumer failed: %d", rc);
        return ESP_FAIL;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    // Release
    report[0] = 0;
    report[1] = 0;
    om = ble_hs_mbuf_from_flat(report, sizeof(report));
    if (om) {
        ble_gatts_notify_custom(g_conn_handle, g_consumer_handle, om);
    }

    return ESP_OK;
}

esp_err_t ble_hid_send_mouse(uint8_t buttons, int8_t dx, int8_t dy,
                              int8_t wheel, int8_t h_wheel)
{
    if (!g_connected || g_mouse_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    // Mouse report: [buttons, dx, dy, wheel, h_wheel] = 5 bytes
    uint8_t report[5] = {
        buttons,
        (uint8_t)dx,
        (uint8_t)dy,
        (uint8_t)wheel,
        (uint8_t)h_wheel
    };

    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    if (!om) {
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_mouse_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notify mouse failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// ============================================================================
// Bond Management APIs
// ============================================================================

int ble_hid_get_bonded_count(void)
{
    if (!g_initialized) {
        return 0;  // Not initialized yet
    }

    ble_addr_t peer_id_addrs[CONFIG_BT_NIMBLE_MAX_BONDS];
    int num_peers = 0;

    int rc = ble_store_util_bonded_peers(peer_id_addrs, &num_peers, CONFIG_BT_NIMBLE_MAX_BONDS);
    if (rc == 0) {
        return num_peers;
    }
    return 0;
}

int ble_hid_get_bonded_devices(uint8_t addrs[][6], int max_count)
{
    if (!g_initialized || max_count <= 0) {
        return 0;
    }

    ble_addr_t peer_id_addrs[CONFIG_BT_NIMBLE_MAX_BONDS];
    int num_peers = 0;

    int rc = ble_store_util_bonded_peers(peer_id_addrs, &num_peers, CONFIG_BT_NIMBLE_MAX_BONDS);
    if (rc != 0) {
        return 0;
    }

    int count = (num_peers < max_count) ? num_peers : max_count;
    for (int i = 0; i < count; i++) {
        memcpy(addrs[i], peer_id_addrs[i].val, 6);
    }

    return count;
}

esp_err_t ble_hid_delete_bond(const uint8_t *addr)
{
    if (!g_initialized || !addr) {
        return ESP_ERR_INVALID_STATE;
    }

    // If connected, check if we're deleting the connected device's bond
    if (g_connected && g_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(g_conn_handle, &desc);
        if (rc == 0) {
            // Compare addresses (NimBLE stores in little-endian)
            if (memcmp(desc.peer_id_addr.val, addr, 6) == 0) {
                ESP_LOGI(TAG, "Disconnecting before deleting bond...");
                ble_gap_terminate(g_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                // Wait a bit for disconnect to complete
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }

    // Build ble_addr_t from raw address
    ble_addr_t peer_addr;
    peer_addr.type = BLE_ADDR_PUBLIC;  // Assume public address
    memcpy(peer_addr.val, addr, 6);

    int rc = ble_store_util_delete_peer(&peer_addr);
    if (rc != 0) {
        // Try with random address type
        peer_addr.type = BLE_ADDR_RANDOM;
        rc = ble_store_util_delete_peer(&peer_addr);
    }

    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to delete bond: %d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Bond deleted: %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return ESP_OK;
}

esp_err_t ble_hid_delete_all_bonds(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Check if there are any bonds to clear
    int count = ble_hid_get_bonded_count();
    if (count == 0) {
        ESP_LOGI(TAG, "No bonds to clear");
        return ESP_OK;
    }

    // First disconnect if connected
    if (g_connected) {
        ble_hid_disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    int rc = ble_store_clear();
    if (rc != 0 && rc != 8) {  // 8 = BLE_HS_EREJECT (may occur if already empty)
        ESP_LOGE(TAG, "Failed to clear bonds: %d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "All bonds deleted (count was %d)", count);
    return ESP_OK;
}

esp_err_t ble_hid_disconnect(void)
{
    if (!g_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    int rc = ble_gap_terminate(g_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc != 0) {
        ESP_LOGE(TAG, "Disconnect failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ble_hid_set_auto_reconnect(bool enable)
{
    g_auto_reconnect = enable;
}

bool ble_hid_get_auto_reconnect(void)
{
    return g_auto_reconnect;
}

void ble_hid_register_callback(ble_hid_event_cb_t cb)
{
    g_event_callback = cb;
}
