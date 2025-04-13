/* Openai WebRTC header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _OPENAI_WEBRTC_H_
#define _OPENAI_WEBRTC_H_

#include "esp_webrtc.h"
#include "openai.h"

/**
 * @brief  OpenAI signaling configuration
 *
 * @note   Details see:
 * https://platform.openai.com/docs/api-reference/realtime-sessions/create
 */
typedef struct {
    char *token; /*!< OpenAI token */
} openai_signaling_cfg_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Get OpenAI signaling implementation
 *
 * @return
 *      - NULL    Not enough memory
 *      - Others  OpenAI signaling implementation
 */
const esp_peer_signaling_impl_t *esp_signaling_get_openai_signaling(void);

/**
 * @brief  Start WebRTC
 * @param[in]  openai_ctx OpenAI context
 *
 * @return
 *      - 0       On success
 *      - Others  Fail to start
 */
int start_webrtc(openai_ctx_t *openai_ctx);

/**
 * @brief  Query WebRTC status
 */
void query_webrtc(void);

/**
 * @brief  Start WebRTC
 *
 * @param[in]  url  Signaling URL
 *
 * @return
 *      - 0       On success
 *      - Others  Fail to start
 */
int stop_webrtc(void);

#ifdef __cplusplus
}
#endif

#endif
