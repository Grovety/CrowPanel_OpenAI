#include "nvs.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "nvs";

int nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret == ESP_FAIL) {
        ESP_LOGE(TAG, "Unable to init nvs_flash");
        return -1;
    }
    return 0;
}

int nvs_store_str(const char *part, const char *key, const char *str) {
    int success = 0;
    nvs_handle_t nvs = 0;
    do {
        esp_err_t ret = nvs_open(part, NVS_READWRITE, &nvs);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Fail to open nvs ret %d", ret);
            break;
        }
        ret = nvs_set_str(nvs, key, (char *)str);
        if (ret != ESP_OK) {
            break;
        }
        success = 1;
    } while (0);
    if (nvs) {
        nvs_close(nvs);
    }
    return success;
}

int nvs_load_str(const char *part, const char *key, char **ret_buf) {
    int success = 0;
    if (!ret_buf) {
        return success;
    }
    nvs_handle_t nvs = 0;
    size_t len = 0;
    do {
        esp_err_t ret = nvs_open(part, NVS_READWRITE, &nvs);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Fail to open nvs ret %d", ret);
            break;
        }
        ret = nvs_get_str(nvs, key, NULL, &len);
        if (ret != ESP_OK || len == 0) {
            break;
        }
        char *buf = malloc(len);
        ret = nvs_get_str(nvs, key, buf, &len);
        *ret_buf = buf;
        success = 1;
    } while (0);
    if (nvs) {
        nvs_close(nvs);
    }
    return success;
}

int nvs_erase_data(const char *part) {
    nvs_handle_t nvs = 0;
    int success = 0;
    do {
        esp_err_t ret = nvs_open(part, NVS_READWRITE, &nvs);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Fail to open nvs ret %d", ret);
            break;
        }
        ret = nvs_erase_all(nvs);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Fail to erase nvs ret %d", ret);
            break;
        }
        success = 1;
    } while (0);
    if (nvs) {
        nvs_close(nvs);
    }
    return success;
}
