/**
 * @file network.h
 * @brief WiFi network connection management via ESP-Hosted
 *
 * Provides WiFi STA connection functionality using the ESP-Hosted
 * coprocessor. Credentials are stored in NVS for persistence.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network connection state
 */
typedef enum {
    NETWORK_STATE_DISCONNECTED = 0,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_CONNECTED,
    NETWORK_STATE_ERROR,
} network_state_t;

/**
 * @brief Network event callback type
 */
typedef void (*network_event_cb_t)(network_state_t state);

/**
 * @brief Initialize network subsystem
 *
 * Initializes WiFi STA mode via ESP-Hosted. Does not connect automatically.
 * Call network_connect() after initialization to establish connection.
 *
 * @return ESP_OK on success
 */
esp_err_t network_init(void);

/**
 * @brief Deinitialize network subsystem
 *
 * @return ESP_OK on success
 */
esp_err_t network_deinit(void);

/**
 * @brief Connect to WiFi access point
 *
 * @param[in] ssid WiFi SSID (max 32 chars)
 * @param[in] password WiFi password (max 64 chars)
 * @return ESP_OK if connection started, ESP_FAIL otherwise
 */
esp_err_t network_connect(const char *ssid, const char *password);

/**
 * @brief Connect using saved credentials from NVS
 *
 * @return ESP_OK if connection started, ESP_ERR_NOT_FOUND if no saved credentials
 */
esp_err_t network_connect_saved(void);

/**
 * @brief Disconnect from WiFi
 *
 * @return ESP_OK on success
 */
esp_err_t network_disconnect(void);

/**
 * @brief Check if connected to WiFi
 *
 * @return true if connected
 */
bool network_is_connected(void);

/**
 * @brief Get current network state
 *
 * @return Current network state
 */
network_state_t network_get_state(void);

/**
 * @brief Get IP address as string
 *
 * @param[out] ip_str Buffer to store IP address string (min 16 bytes)
 * @param[in] len Buffer length
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not connected
 */
esp_err_t network_get_ip(char *ip_str, size_t len);

/**
 * @brief Save WiFi credentials to NVS
 *
 * @param[in] ssid WiFi SSID
 * @param[in] password WiFi password
 * @return ESP_OK on success
 */
esp_err_t network_save_credentials(const char *ssid, const char *password);

/**
 * @brief Clear saved WiFi credentials from NVS
 *
 * @return ESP_OK on success
 */
esp_err_t network_clear_credentials(void);

/**
 * @brief Check if credentials are saved in NVS
 *
 * @return true if credentials exist
 */
bool network_has_saved_credentials(void);

/**
 * @brief Register network event callback
 *
 * @param[in] cb Callback function
 */
void network_register_callback(network_event_cb_t cb);

#ifdef __cplusplus
}
#endif
