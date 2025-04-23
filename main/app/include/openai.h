/* Openai header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _OPENAI_H_
#define _OPENAI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"

#include "esp_webrtc.h"

#include <cJSON.h>

#define OPENAI_URL_ROOT       "https://api.openai.com"
#define OPENAI_CHAT_MODEL     "gpt-4.1-mini"
#define OPENAI_REALTIME_MODEL "gpt-4o-mini-transcribe"
#define OPENAI_REALTIME_URL   OPENAI_URL_ROOT "/v1/realtime"
#define OPENAI_REALTIME_SESSION_URL                                            \
    OPENAI_REALTIME_URL "/transcription_sessions"
#define OPENAI_API_URL     OPENAI_URL_ROOT "/v1/responses"
#define OPENAI_AUTH_PREFIX "Authorization: Bearer "

typedef enum {
    OPENAI_EVENT_NONE = 0, /*!< None event */
    OPENAI_EVENT_TRANSCRIPTION_DONE = 1,
    OPENAI_EVENT_AUDIO_COMMITTED = 2,
    OPENAI_EVENT_REQUEST_SENT = 3,
    OPENAI_EVENT_REQUEST_TIMEOUT = 4,
    OPENAI_EVENT_RESPONSE_DONE = 5,
} openai_event_type_t;

typedef struct {
    openai_event_type_t type; /*!< Event type */
    void *user_data;          /*!< User data (maybe NULL) */
} openai_event_t;

typedef struct {
    /**
     * @brief  Token for OpenAI API
     */
    char api_key[256];
    /**
     * @brief  Buffer for last response id to keep conversation context
     */
    char previous_response_id[64];
} openai_session_ctx_t;

typedef struct {
    /**
     * @brief  OpenAI webrtc events queue
     */
    QueueHandle_t webrtc_event_queue;
    /**
     * @brief  OpenAI general events queue
     */
    QueueHandle_t openai_event_queue;
    /**
     * @brief  Buffer for messages from OpenAI API
     */
    MessageBufferHandle_t messages_buffer;
    /**
     * @brief  Buffer for transctiptions from voice messages sent to OpenAI
     */
    MessageBufferHandle_t transcripts_buffer;
    /**
     * @brief  Current session ctx
     */
    openai_session_ctx_t session;
} openai_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Init OpenAI context
 *
 * @param[out]  ctx Storage for OpenAI context pointer
 *
 * @return
 *      - 0       On success
 *      - Others  Fail to init
 */
int openai_init(openai_ctx_t **ctx);
/**
 * @brief  Reset OpenAI context
 *
 * @param[out]  ctx Storage for OpenAI context pointer
 *
 * @return
 *      - 0       On success
 *      - Others  Fail to init
 */
int openai_reset(openai_ctx_t *ctx);
/**
 * @brief  Send text to OpenAI server
 *
 * @param[in]  ctx  OpenAI context
 * @param[in]  text  Text to be sent
 *
 * @return
 *      - 0       On success
 *      - Others  Fail to start
 */
int openai_send_text(openai_ctx_t *ctx, char *text);

#ifdef __cplusplus
}
#endif

#endif
