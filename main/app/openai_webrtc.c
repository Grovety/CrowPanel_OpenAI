/* OpenAI realtime communication Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_webrtc.h"
#include "media_lib_os.h"
#include "media_sys.h"
#include "openai_webrtc.h"

#include "esp_log.h"

#include "esp_peer_default.h"
#include "esp_webrtc_defaults.h"

#define TAG "OPENAI_WEBRTC"

static esp_webrtc_handle_t webrtc = NULL;

static int webrtc_data_handler(esp_webrtc_custom_data_via_t via, uint8_t *data,
                               int size, void *ctx) {
    openai_ctx_t *openai_ctx = (openai_ctx_t *)ctx;

    cJSON *root = cJSON_Parse((const char *)data);
    if (!root) {
        return -1;
    }
    char *text = cJSON_Print(root);
    if (text) {
        ESP_LOGD(__FUNCTION__, "recv:\n%s", text);
        free(text);
    }

    do {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
        if (!cJSON_IsString(type)) {
            break;
        }

        if (strcmp(type->valuestring,
                   "conversation.item.input_audio_transcription."
                   "completed") == 0) {
            cJSON *transcript =
                cJSON_GetObjectItemCaseSensitive(root, "transcript");
            if (transcript && cJSON_IsString(transcript)) {
                ESP_LOGI(TAG, "Transcript:\n%s", transcript->valuestring);
                xMessageBufferSend(openai_ctx->transcripts_buffer,
                                   transcript->valuestring,
                                   strlen(transcript->valuestring), 0);
                openai_event_t ev = {
                    .type = OPENAI_EVENT_TRANSCRIPTION_DONE,
                    .user_data = NULL,
                };
                xQueueSend(openai_ctx->openai_event_queue, &ev, 0);
                openai_send_text(openai_ctx, transcript->valuestring);
            }
        } else if (strcmp(type->valuestring, "transcription_session.created") ==
                   0) {
            // here webrtc session can be updated
        }
    } while (false);
    cJSON_Delete(root);
    return 0;
}

static int webrtc_event_handler(esp_webrtc_event_t *event, void *ctx) {
    openai_ctx_t *openai_ctx = (openai_ctx_t *)ctx;
    xQueueSend(openai_ctx->webrtc_event_queue, event, 0);
    printf("====================Event %d======================\n", event->type);
    return 0;
}

int start_webrtc(openai_ctx_t *openai_ctx) {
    if (webrtc) {
        esp_webrtc_close(webrtc);
        webrtc = NULL;
    }
    esp_peer_default_cfg_t peer_cfg = {
        .agent_recv_timeout = 500,
    };
    openai_signaling_cfg_t openai_cfg = {
        .token = openai_ctx->api_key,
    };
    esp_webrtc_cfg_t cfg = {
        .peer_cfg =
            {
                .audio_info =
                    {
                        .codec = ESP_PEER_AUDIO_CODEC_OPUS,
                        .sample_rate = 16000,
                        .channel = 1,
                    },
                .audio_dir = ESP_PEER_MEDIA_DIR_SEND_ONLY,
                .enable_data_channel = true,
                .on_custom_data = webrtc_data_handler,
                .ctx = openai_ctx,
                .extra_cfg = &peer_cfg,
                .extra_size = sizeof(peer_cfg),
            },
        .signaling_cfg.extra_cfg = &openai_cfg,
        .peer_impl = esp_peer_get_default_impl(),
        .signaling_impl = esp_signaling_get_openai_signaling(),
    };
    int ret = esp_webrtc_open(&cfg, &webrtc);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to open webrtc");
        return ret;
    }
    // Set media provider
    esp_webrtc_media_provider_t media_provider = {};
    media_sys_get_provider(&media_provider);
    esp_webrtc_set_media_provider(webrtc, &media_provider);

    // Set event handler
    esp_webrtc_set_event_handler(webrtc, webrtc_event_handler, openai_ctx);

    // Start webrtc
    ret = esp_webrtc_start(webrtc);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to start webrtc");
    }
    return ret;
}

void query_webrtc(void) {
    if (webrtc) {
        esp_webrtc_query(webrtc);
    }
}

int stop_webrtc(void) {
    if (webrtc) {
        esp_webrtc_handle_t handle = webrtc;
        webrtc = NULL;
        esp_webrtc_close(handle);
    }
    return 0;
}
