#include "wifi.h"

#include "string.h"

#include "esp_log.h"

static const char *TAG = "network";

#define WIFI_SCAN_LIST_SIZE 10

static wifi_ap_record_t app_info_list[WIFI_SCAN_LIST_SIZE];

void wifi_start_scan() {
    wifi_get_available_aps(app_info_list, WIFI_SCAN_LIST_SIZE);
}

int get_available_networks(char **buffer) {
    if (!buffer) {
        return -1;
    }
    size_t total_len = 0;
    for (size_t i = 0; i < WIFI_SCAN_LIST_SIZE; i++) {
        total_len += strlen((const char *)app_info_list[i].ssid) + 1;
    }
    ESP_LOGD(TAG, "total_len=%u", total_len);
    *buffer = malloc(total_len);
    if (!*buffer) {
        return -1;
    }
    char *ptr = *buffer;
    for (int i = 0; i < WIFI_SCAN_LIST_SIZE; i++) {
        size_t len = strlen((const char *)app_info_list[i].ssid) + 1;
        memcpy(ptr, app_info_list[i].ssid, len);
        ptr += len;
    }
    ESP_LOG_BUFFER_HEXDUMP(TAG, *buffer, total_len, ESP_LOG_DEBUG);
    return WIFI_SCAN_LIST_SIZE;
}
