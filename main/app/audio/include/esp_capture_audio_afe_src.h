/* esp capture with Audio Front End interface header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _ESP_CAPTURE_AUDIO_AFE_SRC_H_
#define _ESP_CAPTURE_AUDIO_AFE_SRC_H_

#include "esp_capture_audio_src_if.h"
#include "esp_capture_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Audio AFE source configuration
 */
typedef struct {
    void *record_handle; /*!< Record handle of `esp_codec_dev` */
} esp_capture_audio_afe_src_cfg_t;

/**
 * @brief  Create audio source instance for AFE
 *
 * @param[in]  cfg  Audio AFE source configuration
 *
 * @return
 *       - NULL    Not enough memory to hold audio codec source instance
 *       - Others  Audio codec source instance
 *
 */
esp_capture_audio_src_if_t *
esp_capture_new_audio_afe_src(esp_capture_audio_afe_src_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
