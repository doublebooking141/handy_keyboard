/**
 * @file keyboard_info.h
 * @brief Handy Keyboard - ハードウェア情報定義
 *
 * デバイスメタデータ、USB識別情報、ファームウェアバージョンを定義
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief デバイス基本情報
 */
#define DEVICE_NAME             "Handy Keyboard"
#define DEVICE_MANUFACTURER     "doublebooking"
#define DEVICE_MAINTAINER       "doublebooking"
#define DEVICE_URL              "https://github.com/doublebooking141"

/**
 * @brief USB識別情報
 * Note: sdkconfig.defaults の TinyUSB 設定と一致させる
 */
#define USB_VID                 0xCAFE      ///< Vendor ID
#define USB_PID                 0x4001      ///< Product ID (Handy Keyboard)
#define USB_DEVICE_VERSION      0x0100      ///< Device Version (BCD: 1.0.0)

/**
 * @brief USB文字列（String Descriptors用）
 */
#define USB_MANUFACTURER_STRING DEVICE_MANUFACTURER
#define USB_PRODUCT_STRING      DEVICE_NAME
#define USB_SERIAL_STRING       "HK20260112"  ///< シリアル番号

/**
 * @brief ファームウェアバージョン
 */
#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_STRING "0.1.0"

/**
 * @brief ハードウェア情報（ESP32-P4 + ESP32-C6）
 */
#define HW_MCU_MAIN            "ESP32-P4"      ///< メインMCU（RISC-V dual-core 400MHz）
#define HW_MCU_WIRELESS        "ESP32-C6"      ///< 無線MCU（WiFi 6 + BLE 5.4）
#define HW_DISPLAY_SIZE        "4.3\""         ///< ディスプレイサイズ
#define HW_DISPLAY_RESOLUTION  "480x800"       ///< ディスプレイ解像度
#define HW_DISPLAY_TYPE        "MIPI-DSI"      ///< ディスプレイインターフェース

/**
 * @brief I2C周辺デバイス情報
 */
#define HW_I2C_SDA_GPIO        7               ///< I2C SDA (GPIO7)
#define HW_I2C_SCL_GPIO        8               ///< I2C SCL (GPIO8)
#define HW_I2C_FREQ_HZ         400000          ///< I2C クロック周波数 (400kHz)

// I2Cデバイスアドレス
#define HW_I2C_ADDR_ES8311     0x18            ///< ES8311 Audio Codec
#define HW_I2C_ADDR_GT911      0x5D            ///< GT911 Touch Controller
#define HW_I2C_ADDR_DS3231M    0x68            ///< DS3231M RTC

/**
 * @brief メモリ構成
 */
#define HW_FLASH_SIZE_MB       16              ///< Flash サイズ (16MB)
#define HW_PSRAM_SIZE_MB       32              ///< PSRAM サイズ (32MB)

#ifdef __cplusplus
}
#endif
