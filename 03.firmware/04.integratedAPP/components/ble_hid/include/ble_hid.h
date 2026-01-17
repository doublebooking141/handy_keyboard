/**
 * @file ble_hid.h
 * @brief BLE HID Keyboard Public API
 *
 * Provides BLE HID keyboard functionality via ESP-Hosted + NimBLE.
 * Supports keyboard reports, consumer control, and Japanese flick input.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE HID connection state
 */
typedef enum {
    BLE_HID_STATE_IDLE,
    BLE_HID_STATE_ADVERTISING,
    BLE_HID_STATE_CONNECTED,
    BLE_HID_STATE_DISCONNECTED,
} ble_hid_state_t;

/**
 * @brief BLE HID event callback type
 */
typedef void (*ble_hid_event_cb_t)(ble_hid_state_t state);

/**
 * @brief Initialize BLE HID subsystem
 *
 * Initializes ESP-Hosted, NimBLE, and esp_hidd for BLE keyboard functionality.
 *
 * @return ESP_OK on success
 */
esp_err_t ble_hid_init(void);

/**
 * @brief Deinitialize BLE HID subsystem
 *
 * @return ESP_OK on success
 */
esp_err_t ble_hid_deinit(void);

/**
 * @brief Start BLE advertising
 *
 * Makes the device discoverable and connectable.
 *
 * @return ESP_OK on success
 */
esp_err_t ble_hid_start_advertising(void);

/**
 * @brief Stop BLE advertising
 *
 * @return ESP_OK on success
 */
esp_err_t ble_hid_stop_advertising(void);

/**
 * @brief Send keyboard report
 *
 * @param modifiers Modifier keys (Ctrl, Shift, Alt, GUI)
 * @param keys Array of up to 6 keycodes
 * @param key_count Number of keys in array
 * @return ESP_OK on success
 */
esp_err_t ble_hid_send_keyboard(uint8_t modifiers, const uint8_t *keys, size_t key_count);

/**
 * @brief Send key press and release
 *
 * Convenience function to send a single key press followed by release.
 *
 * @param modifiers Modifier keys
 * @param keycode HID keycode
 * @return ESP_OK on success
 */
esp_err_t ble_hid_send_key(uint8_t modifiers, uint8_t keycode);

/**
 * @brief Send string as keyboard input
 *
 * Sends each character as a key press/release sequence.
 *
 * @param str ASCII string to send
 * @return ESP_OK on success
 */
esp_err_t ble_hid_send_string(const char *str);

/**
 * @brief Send consumer control report (media keys)
 *
 * @param usage Consumer usage code (e.g., volume up/down)
 * @return ESP_OK on success
 */
esp_err_t ble_hid_send_consumer(uint16_t usage);

// ============================================================================
// Mouse Constants and API
// ============================================================================

/** @brief Mouse button definitions */
#define MOUSE_BTN_LEFT          0x01
#define MOUSE_BTN_RIGHT         0x02
#define MOUSE_BTN_MIDDLE        0x04

/**
 * @brief Send mouse report
 *
 * @param buttons Button state (MOUSE_BTN_LEFT, MOUSE_BTN_RIGHT, MOUSE_BTN_MIDDLE)
 * @param dx X movement (-127 to 127, positive = right)
 * @param dy Y movement (-127 to 127, positive = down)
 * @param wheel Vertical scroll (-127 to 127, positive = up)
 * @param h_wheel Horizontal scroll (-127 to 127, positive = right)
 * @return ESP_OK on success
 */
esp_err_t ble_hid_send_mouse(uint8_t buttons, int8_t dx, int8_t dy,
                              int8_t wheel, int8_t h_wheel);

/**
 * @brief Get current BLE HID state
 *
 * @return Current connection state
 */
ble_hid_state_t ble_hid_get_state(void);

/**
 * @brief Check if BLE HID is connected
 *
 * @return true if connected to a host
 */
bool ble_hid_is_connected(void);

/**
 * @brief Register event callback
 *
 * @param cb Callback function
 */
void ble_hid_register_callback(ble_hid_event_cb_t cb);

// ============================================================================
// Bond Management APIs
// ============================================================================

/**
 * @brief Get number of bonded devices
 *
 * @return Number of bonded devices (0 to CONFIG_BT_NIMBLE_MAX_BONDS)
 */
int ble_hid_get_bonded_count(void);

/**
 * @brief Get bonded device addresses
 *
 * @param addrs Array to store addresses (caller allocates)
 * @param max_count Maximum number of addresses to retrieve
 * @return Number of addresses retrieved
 */
int ble_hid_get_bonded_devices(uint8_t addrs[][6], int max_count);

/**
 * @brief Delete a specific bonded device
 *
 * @param addr 6-byte Bluetooth address of device to delete
 * @return ESP_OK on success
 */
esp_err_t ble_hid_delete_bond(const uint8_t *addr);

/**
 * @brief Delete all bonded devices
 *
 * Clears all bonding information from NVS.
 *
 * @return ESP_OK on success
 */
esp_err_t ble_hid_delete_all_bonds(void);

/**
 * @brief Disconnect from current device
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not connected
 */
esp_err_t ble_hid_disconnect(void);

/**
 * @brief Enable/disable auto-reconnect
 *
 * When enabled, automatically starts advertising after disconnection.
 *
 * @param enable true to enable, false to disable
 */
void ble_hid_set_auto_reconnect(bool enable);

/**
 * @brief Check if auto-reconnect is enabled
 *
 * @return true if auto-reconnect is enabled
 */
bool ble_hid_get_auto_reconnect(void);

#ifdef __cplusplus
}
#endif
