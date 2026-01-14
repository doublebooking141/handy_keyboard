/**
 * @file ble_hid_keycodes.h
 * @brief USB HID Keyboard Keycodes
 *
 * Based on USB HID Usage Tables 1.12
 * https://usb.org/sites/default/files/hut1_12.pdf
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Modifier Keys (bitmap)
// ============================================================================

#define HID_MOD_NONE    0x00
#define HID_MOD_LCTRL   0x01
#define HID_MOD_LSHIFT  0x02
#define HID_MOD_LALT    0x04
#define HID_MOD_LGUI    0x08
#define HID_MOD_RCTRL   0x10
#define HID_MOD_RSHIFT  0x20
#define HID_MOD_RALT    0x40
#define HID_MOD_RGUI    0x80

// ============================================================================
// Letter Keys (US Layout)
// ============================================================================

#define HID_KEY_A       0x04
#define HID_KEY_B       0x05
#define HID_KEY_C       0x06
#define HID_KEY_D       0x07
#define HID_KEY_E       0x08
#define HID_KEY_F       0x09
#define HID_KEY_G       0x0A
#define HID_KEY_H       0x0B
#define HID_KEY_I       0x0C
#define HID_KEY_J       0x0D
#define HID_KEY_K       0x0E
#define HID_KEY_L       0x0F
#define HID_KEY_M       0x10
#define HID_KEY_N       0x11
#define HID_KEY_O       0x12
#define HID_KEY_P       0x13
#define HID_KEY_Q       0x14
#define HID_KEY_R       0x15
#define HID_KEY_S       0x16
#define HID_KEY_T       0x17
#define HID_KEY_U       0x18
#define HID_KEY_V       0x19
#define HID_KEY_W       0x1A
#define HID_KEY_X       0x1B
#define HID_KEY_Y       0x1C
#define HID_KEY_Z       0x1D

// ============================================================================
// Number Keys
// ============================================================================

#define HID_KEY_1       0x1E
#define HID_KEY_2       0x1F
#define HID_KEY_3       0x20
#define HID_KEY_4       0x21
#define HID_KEY_5       0x22
#define HID_KEY_6       0x23
#define HID_KEY_7       0x24
#define HID_KEY_8       0x25
#define HID_KEY_9       0x26
#define HID_KEY_0       0x27

// ============================================================================
// Control Keys
// ============================================================================

#define HID_KEY_ENTER       0x28
#define HID_KEY_ESC         0x29
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_TAB         0x2B
#define HID_KEY_SPACE       0x2C
#define HID_KEY_MINUS       0x2D
#define HID_KEY_EQUAL       0x2E
#define HID_KEY_LBRACKET    0x2F
#define HID_KEY_RBRACKET    0x30
#define HID_KEY_BACKSLASH   0x31
#define HID_KEY_SEMICOLON   0x33
#define HID_KEY_APOSTROPHE  0x34
#define HID_KEY_GRAVE       0x35
#define HID_KEY_COMMA       0x36
#define HID_KEY_DOT         0x37
#define HID_KEY_SLASH       0x38
#define HID_KEY_CAPSLOCK    0x39

// ============================================================================
// Function Keys
// ============================================================================

#define HID_KEY_F1      0x3A
#define HID_KEY_F2      0x3B
#define HID_KEY_F3      0x3C
#define HID_KEY_F4      0x3D
#define HID_KEY_F5      0x3E
#define HID_KEY_F6      0x3F
#define HID_KEY_F7      0x40
#define HID_KEY_F8      0x41
#define HID_KEY_F9      0x42
#define HID_KEY_F10     0x43
#define HID_KEY_F11     0x44
#define HID_KEY_F12     0x45

// ============================================================================
// Navigation Keys
// ============================================================================

#define HID_KEY_PRINTSCREEN 0x46
#define HID_KEY_SCROLLLOCK  0x47
#define HID_KEY_PAUSE       0x48
#define HID_KEY_INSERT      0x49
#define HID_KEY_HOME        0x4A
#define HID_KEY_PAGEUP      0x4B
#define HID_KEY_DELETE      0x4C
#define HID_KEY_END         0x4D
#define HID_KEY_PAGEDOWN    0x4E
#define HID_KEY_RIGHT       0x4F
#define HID_KEY_LEFT        0x50
#define HID_KEY_DOWN        0x51
#define HID_KEY_UP          0x52

// ============================================================================
// Consumer Control Usage Codes
// ============================================================================

#define HID_CONSUMER_PLAY_PAUSE     0x00CD
#define HID_CONSUMER_SCAN_NEXT      0x00B5
#define HID_CONSUMER_SCAN_PREV      0x00B6
#define HID_CONSUMER_STOP           0x00B7
#define HID_CONSUMER_VOLUME_UP      0x00E9
#define HID_CONSUMER_VOLUME_DOWN    0x00EA
#define HID_CONSUMER_MUTE           0x00E2
#define HID_CONSUMER_BRIGHTNESS_UP  0x006F
#define HID_CONSUMER_BRIGHTNESS_DN  0x0070

// ============================================================================
// Character to Keycode Conversion
// ============================================================================

/**
 * @brief Convert ASCII character to HID keycode
 *
 * @param c ASCII character
 * @param[out] keycode HID keycode
 * @param[out] shift true if Shift modifier needed
 * @return true if conversion successful
 */
bool ascii_to_hid_keycode(char c, uint8_t *keycode, bool *shift);

#ifdef __cplusplus
}
#endif
