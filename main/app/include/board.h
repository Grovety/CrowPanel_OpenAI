/* Board audio init header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "esp_codec_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize board
 */
void init_board(void);

/**
 * @brief  Get `esp_codec_dev` handle for playback
 *
 * @return
 *       - NULL    Fail to get playback handle
 *       - Others  Playback `esp_codec_dev` handle
 */
esp_codec_dev_handle_t get_playback_handle(void);

/**
 * @brief  Get `esp_codec_dev` handle for record
 *
 * @return
 *       - NULL    Fail to get record handle
 *       - Others  Record `esp_codec_dev` handle
 */
esp_codec_dev_handle_t get_record_handle(void);

#ifdef __cplusplus
}
#endif

#endif
