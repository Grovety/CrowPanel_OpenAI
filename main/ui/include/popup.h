/* Popup windows header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _POPUP_H_
#define _POPUP_H_

#include "lvgl.h"

typedef struct {
    lv_obj_t *window;
    lv_obj_t *spinner;
} processing_popup_t;

/**
 * @brief  Create processing popup widget
 *
 * @param[in]  cont  Parent container
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  processing popup instance
 */
processing_popup_t *processing_popup_create(lv_obj_t *cont);
/**
 * @brief  Show/hide processing state widget
 *
 * @param[in]  obj    Pointer to processing popup object
 * @param[in]  state  Show/hide state (1, 0)
 */
void processing_popup_show(processing_popup_t *obj, uint8_t state);
/**
 * @brief  Create in-place message box
 *
 * @param[in]  message  C-string message to display
 */
void msg_box_create(const char *message);

#endif
