/**
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in
 * which case, it is free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "esp_capture_audio_afe_src.h"
#include "esp_capture_audio_src_if.h"
#include "esp_capture_types.h"
#include "esp_codec_dev.h"
#include "media_lib_os.h"

#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"

#include "esp_nsn_iface.h"
#include "esp_nsn_models.h"
#include "limiter.h"
#include "model_path.h"

#include "fifo_ringbuf.h"

#define TAG "AFE_SRC"

typedef struct {
    esp_capture_audio_src_if_t base;
    esp_codec_dev_handle_t handle;
    esp_capture_audio_info_t info;
    int frame_num;
    uint64_t frames;
    bool start;
    bool open;

    struct {
        uint8_t feed_stopped;
        uint8_t fetch_stopped;
        uint8_t feed_stop;
        uint8_t fetch_stop;
    } flags;
    const esp_afe_sr_iface_t *afe_if;
    esp_afe_sr_data_t *afe_data;
    fifo_ringbuf_t *audio_ringbuf;
    size_t audio_frame_len;
    size_t audio_frame_sz;
    LimiterState limiter_state;
} afe_dev_src_t;

static int32_t convert_hw_bytes(uint8_t *buf, size_t bytes_width) {
    int32_t val = (buf[bytes_width - 1] & 0x80) ? -1 : 0;
    for (int b = bytes_width - 1; b >= 0; b--)
        val = (val << 8) | (buf[b] & 0xFF);
    return val;
}

static void afe_feed(void *arg) {
    afe_dev_src_t *src = (afe_dev_src_t *)arg;
    const int feed_chunksize = src->afe_if->get_feed_chunksize(src->afe_data);
    const int feed_nch = src->afe_if->get_total_channel_num(src->afe_data);
    ESP_LOGD(TAG, "feed_chunksize=%d, feed_nch=%d", feed_chunksize, feed_nch);
    const size_t read_sz = feed_chunksize * feed_nch * sizeof(int32_t);
    const size_t conv_sz = feed_chunksize * feed_nch * sizeof(float);
    int32_t *read_buf =
        (int32_t *)heap_caps_malloc(read_sz, MALLOC_CAP_INTERNAL);
    int16_t *feed_buf = (int16_t *)read_buf;
    float *conv_buf = (float *)heap_caps_malloc(conv_sz, MALLOC_CAP_INTERNAL);

    ESP_LOGD(TAG, "feed task start");
    while (!src->flags.feed_stop) {
        int ret = esp_codec_dev_read(src->handle, read_buf, read_sz);
        if (ret != 0) {
            ESP_LOGW(TAG, "Fail to read data %d", ret);
            continue;
        }

        // convert 24bits data to 16bits inplace
        for (size_t i = 0; i < feed_chunksize; i++) {
            int32_t val = convert_hw_bytes((uint8_t *)&read_buf[i] + 1, 3);
            conv_buf[i] = (float)val * (1.f / 256) / (1 << 15);
        }

        processBuffer(&src->limiter_state, conv_buf, feed_chunksize);
        for (size_t j = 0; j < feed_chunksize; j++) {
            feed_buf[j] = (int16_t)(conv_buf[j] * (1 << 15));
        }

        ret = src->afe_if->feed(src->afe_data, feed_buf);

        if (ret < 0) {
            ESP_LOGE(TAG, "Fail to feed data %d", ret);
            break;
        }
    }
    src->flags.feed_stopped = true;
    free(read_buf);
    free(conv_buf);
    ESP_LOGD(TAG, "feed task stopped");
    vTaskDelete(NULL);
}

static void afe_fetch(void *arg) {
    afe_dev_src_t *src = (afe_dev_src_t *)arg;
    const int fetch_chunksize = src->afe_if->get_fetch_chunksize(src->afe_data);
    ESP_LOGD(TAG, "fetch_chunksize=%d", fetch_chunksize);

    ESP_LOGD(TAG, "fetch task start");
    while (!src->flags.fetch_stop) {
        afe_fetch_result_t *res = src->afe_if->fetch(src->afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGW(TAG, "ret_value=%d", res->ret_value);
            continue;
        }
        assert(res->data_size % src->audio_frame_sz == 0);

        for (size_t i = 0; i < fetch_chunksize / src->audio_frame_len; i++) {
            size_t bytes = fifo_ringbuf_write(
                src->audio_ringbuf, &res->data[src->audio_frame_len * i],
                src->audio_frame_sz);
            ESP_LOGD(__FUNCTION__, "%lld: wrote [%u]: %u/%u",
                     esp_timer_get_time(), i, bytes, src->audio_frame_sz);
        }
    }
    src->flags.fetch_stopped = true;
    ESP_LOGD(TAG, "fetch task stopped");
    vTaskDelete(NULL);
}

static int afe_dev_src_open(esp_capture_audio_src_if_t *h) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    if (src->handle == NULL) {
        return ESP_CAPTURE_ERR_NOT_SUPPORTED;
    }
    src->frame_num = 0;
    src->open = true;
    return ESP_CAPTURE_ERR_OK;
}

static int
afe_dev_src_get_support_codecs(esp_capture_audio_src_if_t *src,
                               const esp_capture_codec_type_t **codecs,
                               uint8_t *num) {
    static esp_capture_codec_type_t support_codecs[] = {
        ESP_CAPTURE_CODEC_TYPE_PCM};
    *codecs = support_codecs;
    *num = 1;
    return ESP_CAPTURE_ERR_OK;
}

static int afe_dev_src_negotiate_caps(esp_capture_audio_src_if_t *h,
                                      esp_capture_audio_info_t *in_cap,
                                      esp_capture_audio_info_t *out_caps) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    const esp_capture_codec_type_t *codecs = NULL;
    uint8_t num = 0;
    afe_dev_src_get_support_codecs(h, &codecs, &num);
    for (int i = 0; i < num; i++) {
        if (codecs[i] == in_cap->codec) {
            if (in_cap->channel != 1 || in_cap->bits_per_sample != 16) {
                break;
            }
            memcpy(out_caps, in_cap, sizeof(esp_capture_audio_info_t));
            src->info = *in_cap;
            return ESP_CAPTURE_ERR_OK;
        }
    }
    return ESP_CAPTURE_ERR_NOT_SUPPORTED;
}

static int afe_dev_src_start(esp_capture_audio_src_if_t *h) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = src->info.sample_rate,
        .bits_per_sample = 32,
        .channel = src->info.channel,
        .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
    };
    int ret = esp_codec_dev_open(src->handle, &fs);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to open codec device, ret=%d", ret);
        return ESP_CAPTURE_ERR_NOT_SUPPORTED;
    }
    src->start = true;
    src->frame_num = 0;
    src->frames = 0;

    memset(&src->flags, 0, sizeof(src->flags));

    initLimiter(&src->limiter_state, -60, 5, 50, src->info.sample_rate);

    src->audio_frame_len = (src->info.sample_rate / 1000) * 10;
    src->audio_frame_sz =
        src->audio_frame_len * sizeof(int16_t) * src->info.channel;
    src->audio_ringbuf = fifo_ringbuf_init(8, src->audio_frame_sz);
    if (!src->audio_ringbuf) {
        ESP_LOGE(TAG, "Unable to create audio_ringbuf");
        return ESP_CAPTURE_ERR_NO_MEM;
    }
    ESP_LOGD(TAG, "audio_frame_len=%u, audio_frame_sz=%u", src->audio_frame_len,
             src->audio_frame_sz);

    media_lib_thread_handle_t thread = NULL;
    media_lib_thread_create_from_scheduler(&thread, "afe_fetch", afe_fetch,
                                           src);
    media_lib_thread_create_from_scheduler(&thread, "afe_feed", afe_feed, src);

    return ESP_CAPTURE_ERR_OK;
}

static int afe_dev_src_read_frame(esp_capture_audio_src_if_t *h,
                                  esp_capture_stream_frame_t *frame) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    if (src->start == false) {
        return ESP_CAPTURE_ERR_NOT_SUPPORTED;
    }
    int samples = frame->size / (sizeof(int16_t) * src->info.channel);
    size_t read_total = 0;
    for (size_t i = 0; i < samples / src->audio_frame_len; i++) {
        size_t read_bytes = fifo_ringbuf_read(
            src->audio_ringbuf, frame->data + i * src->audio_frame_sz,
            src->audio_frame_sz, portMAX_DELAY);
        ESP_LOGD(__FUNCTION__, "%lld: read [%u]: %u/%u", esp_timer_get_time(),
                 i, read_bytes, src->audio_frame_sz);
        read_total += read_bytes;
    }
    if (read_total != frame->size) {
        ESP_LOGE(TAG, "Fail to read from AFE, %u/%d", read_total, frame->size);
        return ESP_CAPTURE_ERR_INTERNAL;
    }
    frame->pts = src->frame_num * samples * 1000 / src->info.sample_rate;
    src->frame_num++;
    return ESP_CAPTURE_ERR_OK;
}

static int afe_dev_src_stop(esp_capture_audio_src_if_t *h) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    src->flags.fetch_stop = true;
    while (!src->flags.fetch_stopped) {
        media_lib_thread_sleep(10);
    }
    src->flags.feed_stop = true;
    while (!src->flags.feed_stopped) {
        media_lib_thread_sleep(10);
    }
    src->afe_if->reset_buffer(src->afe_data);
    fifo_ringbuf_reset(src->audio_ringbuf);
    if (src->handle) {
        esp_codec_dev_close(src->handle);
    }
    src->start = false;
    return ESP_CAPTURE_ERR_OK;
}

static int afe_dev_src_close(esp_capture_audio_src_if_t *h) {
    afe_dev_src_t *src = (afe_dev_src_t *)h;
    src->handle = NULL;
    if (src->afe_data) {
        src->afe_if->destroy(src->afe_data);
        src->afe_data = NULL;
    }
    if (src->audio_ringbuf) {
        fifo_ringbuf_release(src->audio_ringbuf);
        src->audio_ringbuf = NULL;
    }
    return ESP_CAPTURE_ERR_OK;
}

esp_capture_audio_src_if_t *
esp_capture_new_audio_afe_src(esp_capture_audio_afe_src_cfg_t *cfg) {
    if (cfg == NULL || cfg->record_handle == NULL) {
        return NULL;
    }
    afe_dev_src_t *src = calloc(1, sizeof(afe_dev_src_t));
    src->base.open = afe_dev_src_open;
    src->base.get_support_codecs = afe_dev_src_get_support_codecs;
    src->base.negotiate_caps = afe_dev_src_negotiate_caps;
    src->base.start = afe_dev_src_start;
    src->base.read_frame = afe_dev_src_read_frame;
    src->base.stop = afe_dev_src_stop;
    src->base.close = afe_dev_src_close;
    src->handle = cfg->record_handle;

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.afe_perferred_priority = 4;
    afe_config.afe_mode = SR_MODE_HIGH_PERF;
    afe_config.aec_init = false;
    afe_config.vad_init = false;
    afe_config.wakenet_init = false;
    afe_config.voice_communication_init = true;
    afe_config.voice_communication_agc_init = true;
    afe_config.voice_communication_agc_gain = 44;
    afe_config.pcm_config.total_ch_num = 1, afe_config.pcm_config.mic_num = 1,
    afe_config.pcm_config.ref_num = 0,
    afe_config.pcm_config.sample_rate = 16000,
    afe_config.afe_ns_mode = NS_MODE_SSP;
    afe_config.afe_ringbuf_size = 16;

    /* srmodel_list_t *models = esp_srmodel_init("model"); */
    /* char *nsnet_name = NULL; */
    /* nsnet_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL); */
    /* printf("nsnet_name: %s\n", nsnet_name ? nsnet_name : ""); */
    /* afe_config.afe_ns_model_name = nsnet_name; */

    src->afe_if = (esp_afe_sr_iface_t *)&ESP_AFE_VC_HANDLE;
    src->afe_data = src->afe_if->create_from_config(&afe_config);
    if (!src->afe_data) {
        ESP_LOGE(TAG, "afe_data is null!");
        return NULL;
    }

    return &src->base;
}
