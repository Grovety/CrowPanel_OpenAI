/* Common header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "lvgl.h"

typedef struct {
    lv_obj_t *cont;
    lv_obj_t *label;
    lv_obj_t *field;
} area_t;

typedef struct {
    lv_obj_t *label;
    lv_obj_t *btn;
} button_t;

extern lv_style_t m_font_style;
extern lv_style_t s_font_style;
extern lv_style_t transparent_area_style;

/**
 * @brief  Init common styles
 */
void ui_styles_init();
/**
 * @brief  Common text area callback
 */
void text_area_event_cb(lv_event_t *e);

#endif
