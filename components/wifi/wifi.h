#ifndef _WIFI_H_
#define _WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"

#define WIFI_STATE_CONNECTED_BIT BIT0
#define WIFI_STATE_UPDATE_BIT    BIT1
#define WIFI_SCAN_DONE_BIT       BIT2

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t wifi_event_group;

/**
 * @brief  Init esp_wifi driver
 *
 * @return
 *      - 0       On success
 *      - Others  Invalid argument
 */
int wifi_init();
/**
 * @brief  Get available access points
 *
 * @param[out]  ap_info  Pointer to pre-allocated array of wifi_ap_record_t
 * @param[out]  number  Number of wifi_ap_record_t in array
 */
void wifi_get_available_aps(wifi_ap_record_t *ap_info, uint16_t number);
/**
 * @brief  Connect to AP with SSID and password
 *
 * @param[in]  ssid  C-string with SSID
 * @param[in]  password  C-string with password
 */
void wifi_connect(const char *ssid, const char *password);
/**
 * @brief  Disonnect from current AP
 */
void wifi_disconnect();

#ifdef __cplusplus
}
#endif

#endif // _WIFI_H_
