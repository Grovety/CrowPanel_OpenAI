/* dummy audio data interface header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _ESP_CODEC_DATA_DUMMY_H_
#define _ESP_CODEC_DATA_DUMMY_H_

#include "audio_codec_data_if.h"
#include "audio_codec_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Codec data configuration
 */
typedef struct {
} audio_codec_data_dummy_cfg_t;

/**
 * @brief         Get dummy codec data interface
 * @return        NULL: Failed
 *                Others: dummy data interface
 */
const audio_codec_data_if_t *
audio_codec_new_dummy_data(audio_codec_data_dummy_cfg_t *default_cfg);

#ifdef __cplusplus
}
#endif

#endif
