/* Settings window header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _SETTINGS_WINDOW_H_
#define _SETTINGS_WINDOW_H_

#include "common.h"

typedef struct {
    button_t clear_storage;
} settings_window_t;

/**
 * @brief  Create settings window
 *
 * @param[in]  cont      Parent container
 * @param[in]  keyboard  Pointer to keyboard object
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  settings window instance
 */
settings_window_t *settings_window_create(lv_obj_t *cont, lv_obj_t *keyboard);

#endif
