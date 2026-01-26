/**
 * @file ntp.h
 * @brief NTP time synchronization with RTC update
 *
 * Synchronizes time from NTP servers and updates both the ESP32
 * internal RTC and the external DS3231M RTC chip.
 */

#pragma once

#include <time.h>
#include <stdbool.h>
#include "esp_err.h"
#include "ds3231m.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NTP server selection
 */
typedef enum {
    NTP_SERVER_NICT = 0,    ///< NICT Japan (ntp.nict.jp)
    NTP_SERVER_GOOGLE,      ///< Google (time.google.com)
    NTP_SERVER_MAX
} ntp_server_t;

/**
 * @brief NTP sync status
 */
typedef enum {
    NTP_STATUS_IDLE = 0,
    NTP_STATUS_SYNCING,
    NTP_STATUS_SYNCED,
    NTP_STATUS_FAILED,
} ntp_status_t;

/**
 * @brief NTP sync callback type
 */
typedef void (*ntp_sync_cb_t)(ntp_status_t status);

/**
 * @brief Initialize NTP subsystem
 *
 * @param[in] rtc_handle DS3231M RTC handle for time writeback (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t ntp_init(ds3231m_handle_t *rtc_handle);

/**
 * @brief Deinitialize NTP subsystem
 *
 * @return ESP_OK on success
 */
esp_err_t ntp_deinit(void);

/**
 * @brief Start NTP synchronization
 *
 * Initiates time sync from the selected NTP server. On success,
 * updates both ESP32 internal RTC and DS3231M external RTC.
 *
 * @param[in] server NTP server to use
 * @return ESP_OK if sync started, ESP_ERR_INVALID_STATE if not connected
 */
esp_err_t ntp_sync(ntp_server_t server);

/**
 * @brief Get current NTP sync status
 *
 * @return Current sync status
 */
ntp_status_t ntp_get_status(void);

/**
 * @brief Set time manually
 *
 * Updates both ESP32 internal RTC and DS3231M external RTC.
 *
 * @param[in] time Time to set
 * @return ESP_OK on success
 */
esp_err_t ntp_set_time_manual(const struct tm *time);

/**
 * @brief Set default NTP server
 *
 * Saves preference to NVS.
 *
 * @param[in] server NTP server to set as default
 * @return ESP_OK on success
 */
esp_err_t ntp_set_default_server(ntp_server_t server);

/**
 * @brief Get default NTP server
 *
 * @return Saved default server
 */
ntp_server_t ntp_get_default_server(void);

/**
 * @brief Get server name string
 *
 * @param[in] server Server enum value
 * @return Server hostname string
 */
const char *ntp_get_server_name(ntp_server_t server);

/**
 * @brief Register NTP sync callback
 *
 * @param[in] cb Callback function
 */
void ntp_register_callback(ntp_sync_cb_t cb);

/**
 * @brief Set NTP sync timeout
 *
 * If sync doesn't complete within timeout, status changes to NTP_STATUS_FAILED.
 *
 * @param[in] timeout_ms Timeout in milliseconds (1000-300000, default 30000)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range
 */
esp_err_t ntp_set_timeout(uint32_t timeout_ms);

/**
 * @brief Get current NTP sync timeout
 *
 * @return Timeout in milliseconds
 */
uint32_t ntp_get_timeout(void);

#ifdef __cplusplus
}
#endif
