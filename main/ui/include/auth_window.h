/* OpenAI API authentification window header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _AUTH_WINDOW_H_
#define _AUTH_WINDOW_H_

#include "common.h"

typedef struct {
    area_t api_key;
    button_t connect;
} auth_window_t;

/**
 * @brief  Create auth window
 *
 * @param[in]  cont  Parent container
 * @param[in]  keyboard  Pointer to keyboard object
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  auth window instance
 */
auth_window_t *auth_window_create(lv_obj_t *cont, lv_obj_t *keyboard);
/**
 * @brief  Set api key in text area
 *
 * @param[in]  win      Pointer to auth window
 * @param[in]  api_key  API key c-string
 */
void set_api_key(auth_window_t *win, const char *api_key);

#endif
