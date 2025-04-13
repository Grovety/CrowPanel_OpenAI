#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"

#include "wifi.h"

#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define ESP_WIFI_MAXIMUM_RETRY 2

static const char *TAG = "wifi";

EventGroupHandle_t wifi_event_group;

void wifi_get_available_aps(wifi_ap_record_t *ap_info, uint16_t number) {
    xEventGroupClearBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
    esp_wifi_scan_start(NULL, true);
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGD(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u",
             ap_count, number);
    for (int i = 0; i < number; i++) {
        ESP_LOGD(TAG, "[%d]: SSID=%s", i, ap_info[i].ssid);
    }
    esp_wifi_scan_stop();
    xEventGroupSetBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "event_base=%s, event_id=%ld", event_base, event_id);
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            // esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(wifi_event_group, WIFI_STATE_CONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, WIFI_STATE_UPDATE_BIT);
            break;
        default:
            break;
        }
    }
    if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_STATE_CONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, WIFI_STATE_UPDATE_BIT);
        } break;
        default:
            break;
        }
    }
}

int wifi_init() {
    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group) {
        ESP_LOGE(TAG, "Unable to create wifi event group");
        return -1;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        return -1;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    return 0;
}

void wifi_connect(const char *ssid, const char *password) {
    if (!ssid || !password) {
        return;
    }
    wifi_config_t wifi_config = {
        .sta =
            {
                .threshold =
                    {
                        .authmode = WIFI_AUTH_WPA2_PSK,
                    },
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
                .sae_h2e_identifier = "",
            },
    };

    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_disconnect() { ESP_ERROR_CHECK(esp_wifi_disconnect()); }
