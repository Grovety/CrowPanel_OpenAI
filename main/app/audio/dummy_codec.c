/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "dummy_codec.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    audio_codec_if_t base;
    esp_codec_dev_sample_info_t fs;
    bool is_open;
    bool enable;
} dummy_codec_t;

/*
 * Customization for codec interface
 */
static int dummy_codec_open(const audio_codec_if_t *h, void *cfg,
                            int cfg_size) {
    /* dummy_codec_cfg_t *codec_cfg = (dummy_codec_cfg_t *) cfg; */
    if (cfg_size != sizeof(dummy_codec_cfg_t)) {
        return -1;
    }
    dummy_codec_t *codec = (dummy_codec_t *)h;
    codec->is_open = true;
    return 0;
}

static bool dummy_codec_is_open(const audio_codec_if_t *h) {
    dummy_codec_t *codec = (dummy_codec_t *)h;
    return codec->is_open;
}

/**
 * @brief Enable can be used to control codec suspend/resume behavior to save
 * power
 */
static int dummy_codec_enable(const audio_codec_if_t *h, bool enable) {
    dummy_codec_t *codec = (dummy_codec_t *)h;
    codec->enable = enable;
    return 0;
}

static int dummy_codec_set_fs(const audio_codec_if_t *h,
                              esp_codec_dev_sample_info_t *fs) {
    dummy_codec_t *codec = (dummy_codec_t *)h;
    memcpy(&codec->fs, fs, sizeof(esp_codec_dev_sample_info_t));
    return 0;
}

static int dummy_codec_mute(const audio_codec_if_t *h, bool mute) { return 0; }

static int dummy_codec_set_vol(const audio_codec_if_t *h, float db) {
    return 0;
}

static int dummy_codec_set_mic_gain(const audio_codec_if_t *h, float db) {
    return 0;
}

static int dummy_codec_mute_mic(const audio_codec_if_t *h, bool mute) {
    return 0;
}

static int dummy_codec_set_reg(const audio_codec_if_t *h, int reg, int value) {
    return 0;
}

static int dummy_codec_get_reg(const audio_codec_if_t *h, int reg, int *value) {
    return 0;
}

static int dummy_codec_close(const audio_codec_if_t *h) {
    dummy_codec_t *codec = (dummy_codec_t *)h;
    // Auto disable when codec closed
    if (codec->enable) {
        dummy_codec_enable(h, false);
    }
    codec->is_open = false;
    return 0;
}

const audio_codec_if_t *dummy_codec_new(dummy_codec_cfg_t *codec_cfg) {
    dummy_codec_t *codec = (dummy_codec_t *)calloc(1, sizeof(dummy_codec_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->base.open = dummy_codec_open;
    codec->base.is_open = dummy_codec_is_open;
    codec->base.enable = dummy_codec_enable;
    codec->base.set_fs = dummy_codec_set_fs;
    codec->base.mute = dummy_codec_mute;
    codec->base.set_vol = dummy_codec_set_vol;
    codec->base.set_mic_gain = dummy_codec_set_mic_gain;
    codec->base.mute_mic = dummy_codec_mute_mic;
    codec->base.set_reg = dummy_codec_set_reg;
    codec->base.get_reg = dummy_codec_get_reg;
    codec->base.close = dummy_codec_close;
    codec->base.open(&codec->base, codec_cfg, sizeof(dummy_codec_cfg_t));
    return &codec->base;
}
