#include "openai.h"

#include "esp_log.h"

#include "string.h"

#define TAG "OPENAI"

int openai_init(openai_ctx_t **ctx) {
    if (!ctx) {
        return -1;
    }
    openai_ctx_t *openai_ctx = (openai_ctx_t *)calloc(1, sizeof(openai_ctx_t));
    if (!openai_ctx) {
        ESP_LOGE(TAG, "Unable to allocate openai ctx");
        return -1;
    }
    openai_ctx->webrtc_event_queue =
        xQueueCreate(5, sizeof(esp_webrtc_event_t));
    if (!openai_ctx->webrtc_event_queue) {
        ESP_LOGE(TAG, "Fail to create webrtc event queue");
        free(openai_ctx);
        return -1;
    }
    openai_ctx->openai_event_queue = xQueueCreate(5, sizeof(openai_event_t));
    if (!openai_ctx->openai_event_queue) {
        ESP_LOGE(TAG, "Fail to create openai event queue");
        vQueueDelete(openai_ctx->webrtc_event_queue);
        free(openai_ctx);
        return -1;
    }
    openai_ctx->messages_buffer = xMessageBufferCreate(512);
    if (!openai_ctx->messages_buffer) {
        ESP_LOGE(TAG, "Fail to create messages buffer");
        vQueueDelete(openai_ctx->webrtc_event_queue);
        free(openai_ctx);
        return -1;
    }
    openai_ctx->transcripts_buffer = xMessageBufferCreate(256);
    if (!openai_ctx->transcripts_buffer) {
        ESP_LOGE(TAG, "Fail to create transcripts buffer");
        vQueueDelete(openai_ctx->webrtc_event_queue);
        vMessageBufferDelete(openai_ctx->messages_buffer);
        free(openai_ctx);
        return -1;
    }
    *ctx = openai_ctx;
    return 0;
}

int openai_reset(openai_ctx_t *ctx) {
    xMessageBufferReset(ctx->messages_buffer);
    xMessageBufferReset(ctx->transcripts_buffer);
    memset(&ctx->session, 0, sizeof(ctx->session));
    return 0;
}
