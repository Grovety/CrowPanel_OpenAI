/* dummy codec header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _DUMMY_CODEC_H_
#define _DUMMY_CODEC_H_

#include "esp_codec_dev.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Customized codec configuration
 */
typedef struct {
} dummy_codec_cfg_t;

/**
 * @brief Customized codec realization
 */
const audio_codec_if_t *dummy_codec_new(dummy_codec_cfg_t *codec_cfg);

#ifdef __cplusplus
}
#endif

#endif
