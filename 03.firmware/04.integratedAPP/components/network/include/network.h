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
#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of scan results to return */
#define MAX_SCAN_RESULTS 10

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
 * @brief WiFi scan result entry
 */
typedef struct {
    char ssid[33];              ///< SSID (null-terminated)
    int8_t rssi;                ///< Signal strength in dBm
    wifi_auth_mode_t authmode;  ///< Authentication mode
} network_scan_result_t;

/**
 * @brief WiFi scan complete callback type
 */
typedef void (*network_scan_cb_t)(network_scan_result_t *results, uint16_t count);

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

/**
 * @brief Start WiFi network scan
 *
 * Initiates an asynchronous scan for available WiFi networks.
 * Results are delivered via the callback.
 *
 * @param[in] callback Function to call with scan results
 * @return ESP_OK if scan started, ESP_ERR_INVALID_STATE if already scanning
 */
esp_err_t network_scan_start(network_scan_cb_t callback);

/**
 * @brief Check if WiFi scan is in progress
 *
 * @return true if scanning
 */
bool network_is_scanning(void);

/**
 * @brief Check if ESP-Hosted coprocessor is available
 *
 * @return true if coprocessor is connected and functional
 */
bool network_is_coprocessor_available(void);

/**
 * @brief Reset ESP-Hosted coprocessor connection
 *
 * Attempts to re-initialize the WiFi subsystem to recover from
 * coprocessor communication errors.
 *
 * @return ESP_OK on success
 */
esp_err_t network_reset_coprocessor(void);

#ifdef __cplusplus
}
#endif
