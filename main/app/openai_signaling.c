/* OpenAI signaling

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_log.h"
#include "https_client.h"
#include "openai_webrtc.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "OPENAI_SIGNALING"

#define SAFE_FREE(p)                                                           \
    if (p) {                                                                   \
        free(p);                                                               \
        p = NULL;                                                              \
    }

#define GET_KEY_END(str, key) get_key_end(str, key, sizeof(key) - 1)

typedef struct {
    esp_peer_signaling_cfg_t cfg;
    uint8_t *remote_sdp;
    int remote_sdp_size;
    char *ephemeral_token;
} openai_signaling_t;

static char *get_key_end(char *str, char *key, int len) {
    char *p = strstr(str, key);
    if (p == NULL) {
        return NULL;
    }
    return p + len;
}

static void session_answer(http_resp_t *resp, void *ctx) {
    openai_signaling_t *sig = (openai_signaling_t *)ctx;
    char *token = GET_KEY_END((char *)resp->data, "\"client_secret\"");
    if (token == NULL) {
        return;
    }
    char *secret = GET_KEY_END(token, "\"value\"");
    if (secret == NULL) {
        return;
    }
    char *s = strchr(secret, '"');
    if (s == NULL) {
        return;
    }
    s++;
    char *e = strchr(s, '"');
    *e = 0;
    sig->ephemeral_token = strdup(s);
    *e = '"';
}

static void get_ephemeral_token(openai_signaling_t *sig, char *token) {
    int len = strlen(OPENAI_AUTH_PREFIX) + strlen(token) + 1;
    char auth[len];
    snprintf(auth, len, OPENAI_AUTH_PREFIX "%s", token);
    char *header[] = {
        "Content-Type: application/json",
        auth,
        NULL,
    };
    cJSON *root = cJSON_CreateObject();
    cJSON *input_audio_transcription = cJSON_CreateObject();
    cJSON_AddStringToObject(input_audio_transcription, "model",
                            OPENAI_REALTIME_MODEL);
    cJSON_AddStringToObject(input_audio_transcription, "language", "en");
    cJSON_AddItemToObject(root, "input_audio_transcription",
                          input_audio_transcription);
    char *json_string = cJSON_Print(root);
    if (json_string) {
        ESP_LOGD(TAG, "post to url=%s:\n%s", OPENAI_REALTIME_SESSION_URL,
                 json_string);
        https_post(OPENAI_REALTIME_SESSION_URL, header, json_string, NULL,
                   session_answer, sig);
        free(json_string);
    }
    cJSON_Delete(root);
}

static int openai_signaling_start(esp_peer_signaling_cfg_t *cfg,
                                  esp_peer_signaling_handle_t *h) {
    openai_signaling_t *sig =
        (openai_signaling_t *)calloc(1, sizeof(openai_signaling_t));
    if (sig == NULL) {
        return ESP_PEER_ERR_NO_MEM;
    }
    openai_signaling_cfg_t *openai_cfg =
        (openai_signaling_cfg_t *)cfg->extra_cfg;
    sig->cfg = *cfg;
    get_ephemeral_token(sig, openai_cfg->token);
    if (sig->ephemeral_token == NULL) {
        free(sig);
        return ESP_PEER_ERR_NOT_SUPPORT;
    }
    *h = sig;
    esp_peer_signaling_ice_info_t ice_info = {
        .is_initiator = true,
    };
    sig->cfg.on_ice_info(&ice_info, sig->cfg.ctx);
    sig->cfg.on_connected(sig->cfg.ctx);
    return ESP_PEER_ERR_NONE;
}

static void openai_sdp_answer(http_resp_t *resp, void *ctx) {
    openai_signaling_t *sig = (openai_signaling_t *)ctx;
    ESP_LOGD(TAG, "Get remote SDP %s", (char *)resp->data);
    SAFE_FREE(sig->remote_sdp);
    sig->remote_sdp = (uint8_t *)malloc(resp->size);
    if (sig->remote_sdp == NULL) {
        ESP_LOGE(TAG, "No enough memory for remote sdp");
        return;
    }
    memcpy(sig->remote_sdp, resp->data, resp->size);
    sig->remote_sdp_size = resp->size;
}

static int openai_signaling_send_msg(esp_peer_signaling_handle_t h,
                                     esp_peer_signaling_msg_t *msg) {
    openai_signaling_t *sig = (openai_signaling_t *)h;
    if (msg->type == ESP_PEER_SIGNALING_MSG_BYE) {

    } else if (msg->type == ESP_PEER_SIGNALING_MSG_SDP) {
        ESP_LOGD(TAG, "Receive local SDP");
        char content_type[32] = "Content-Type: application/sdp";
        char *token = sig->ephemeral_token;
        int len = strlen("Authorization: Bearer ") + strlen(token) + 1;
        char auth[len];
        snprintf(auth, len, "Authorization: Bearer %s", token);
        char *header[] = {
            content_type,
            auth,
            NULL,
        };
        int ret = https_post(OPENAI_REALTIME_URL, header, (char *)msg->data,
                             NULL, openai_sdp_answer, h);
        if (ret != 0 || sig->remote_sdp == NULL) {
            ESP_LOGE(TAG, "Fail to post data to %s", OPENAI_REALTIME_URL);
            return -1;
        }
        esp_peer_signaling_msg_t sdp_msg = {
            .type = ESP_PEER_SIGNALING_MSG_SDP,
            .data = sig->remote_sdp,
            .size = sig->remote_sdp_size,
        };
        sig->cfg.on_msg(&sdp_msg, sig->cfg.ctx);
    }
    return 0;
}

static int openai_signaling_stop(esp_peer_signaling_handle_t h) {
    openai_signaling_t *sig = (openai_signaling_t *)h;
    sig->cfg.on_close(sig->cfg.ctx);
    SAFE_FREE(sig->remote_sdp);
    SAFE_FREE(sig->ephemeral_token);
    SAFE_FREE(sig);
    return 0;
}

const esp_peer_signaling_impl_t *esp_signaling_get_openai_signaling(void) {
    static const esp_peer_signaling_impl_t impl = {
        .start = openai_signaling_start,
        .send_msg = openai_signaling_send_msg,
        .stop = openai_signaling_stop,
    };
    return &impl;
}
