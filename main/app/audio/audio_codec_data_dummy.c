/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "audio_codec_data_dummy.h"
#include "audio_codec_data_if.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <string.h>

#define TAG "dummy data IF"

typedef struct {
    audio_codec_data_if_t base;
    bool is_open;
    esp_codec_dev_sample_info_t fs;
} dummy_data_t;

static int _dummy_data_open(const audio_codec_data_if_t *h, void *data_cfg,
                            int cfg_size) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (h == NULL || data_cfg == NULL ||
        cfg_size != sizeof(audio_codec_data_dummy_cfg_t)) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    dummy_data->is_open = true;
    return ESP_CODEC_DEV_OK;
}

static bool _dummy_data_is_open(const audio_codec_data_if_t *h) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data) {
        return dummy_data->is_open;
    }
    return false;
}

static int _dummy_data_enable(const audio_codec_data_if_t *h,
                              esp_codec_dev_type_t dev_type, bool enable) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (dummy_data->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    return ESP_CODEC_DEV_OK;
}

static int _dummy_data_set_fmt(const audio_codec_data_if_t *h,
                               esp_codec_dev_type_t dev_type,
                               esp_codec_dev_sample_info_t *fs) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (dummy_data->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    memcpy(&dummy_data->fs, fs, sizeof(esp_codec_dev_sample_info_t));
    return ESP_CODEC_DEV_OK;
}

static int _dummy_data_read(const audio_codec_data_if_t *h, uint8_t *data,
                            int size) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (dummy_data->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    return ESP_CODEC_DEV_OK;
}

static int _dummy_data_write(const audio_codec_data_if_t *h, uint8_t *data,
                             int size) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (dummy_data->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    return ESP_CODEC_DEV_OK;
}

static int _dummy_data_close(const audio_codec_data_if_t *h) {
    dummy_data_t *dummy_data = (dummy_data_t *)h;
    if (dummy_data == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    dummy_data->is_open = false;
    return ESP_CODEC_DEV_OK;
}

const audio_codec_data_if_t *
audio_codec_new_dummy_data(audio_codec_data_dummy_cfg_t *dummy_cfg) {
    dummy_data_t *dummy_data = calloc(1, sizeof(dummy_data_t));
    if (dummy_data == NULL) {
        ESP_LOGE(TAG, "No memory for instance");
        return NULL;
    }
    dummy_data->base.open = _dummy_data_open;
    dummy_data->base.is_open = _dummy_data_is_open;
    dummy_data->base.enable = _dummy_data_enable;
    dummy_data->base.read = _dummy_data_read;
    dummy_data->base.write = _dummy_data_write;
    dummy_data->base.set_fmt = _dummy_data_set_fmt;
    dummy_data->base.close = _dummy_data_close;
    int ret = _dummy_data_open(&dummy_data->base, dummy_cfg,
                               sizeof(audio_codec_data_dummy_cfg_t));
    if (ret != 0) {
        free(dummy_data);
        return NULL;
    }
    return &dummy_data->base;
}
