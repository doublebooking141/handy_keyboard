/**
 * @file wol.h
 * @brief Wake-on-LAN magic packet sender
 *
 * Sends WOL magic packets to wake up computers on the network.
 * Magic packet format: 0xFF x 6 + Target MAC x 16
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WOL send result
 */
typedef enum {
    WOL_RESULT_OK = 0,
    WOL_RESULT_NOT_CONNECTED,
    WOL_RESULT_INVALID_MAC,
    WOL_RESULT_SEND_FAILED,
} wol_result_t;

/**
 * @brief Initialize WOL subsystem
 *
 * @return ESP_OK on success
 */
esp_err_t wol_init(void);

/**
 * @brief Deinitialize WOL subsystem
 *
 * @return ESP_OK on success
 */
esp_err_t wol_deinit(void);

/**
 * @brief Send WOL magic packet to specified MAC address
 *
 * @param[in] mac Target MAC address (6 bytes)
 * @return WOL_RESULT_OK on success
 */
wol_result_t wol_send(const uint8_t mac[6]);

/**
 * @brief Send WOL magic packet to default target (from Kconfig)
 *
 * @return WOL_RESULT_OK on success
 */
wol_result_t wol_send_default(void);

/**
 * @brief Parse MAC address string to bytes
 *
 * Supports formats: "AA:BB:CC:DD:EE:FF" or "AA-BB-CC-DD-EE-FF"
 *
 * @param[in] mac_str MAC address string
 * @param[out] mac_bytes Output buffer (6 bytes)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on parse error
 */
esp_err_t wol_parse_mac(const char *mac_str, uint8_t mac_bytes[6]);

/**
 * @brief Get default target MAC address
 *
 * @param[out] mac_bytes Output buffer (6 bytes)
 * @return ESP_OK on success
 */
esp_err_t wol_get_default_mac(uint8_t mac_bytes[6]);

/**
 * @brief Set default target MAC address (saved to NVS)
 *
 * @param[in] mac_bytes MAC address (6 bytes)
 * @return ESP_OK on success
 */
esp_err_t wol_set_default_mac(const uint8_t mac_bytes[6]);

/**
 * @brief Get default MAC as string
 *
 * @param[out] mac_str Output buffer (min 18 bytes)
 * @param[in] len Buffer length
 * @return ESP_OK on success
 */
esp_err_t wol_get_default_mac_str(char *mac_str, size_t len);

#ifdef __cplusplus
}
#endif
