/* Status bar header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _STATUS_BAR_H_
#define _STATUS_BAR_H_

#include "lvgl.h"

typedef struct {
    lv_obj_t *bar;
    lv_obj_t *wifi;
    lv_obj_t *webrtc;
    lv_obj_t *separator;
} status_bar_t;

typedef enum {
    SB_WIFI,
    SB_WEBRTC,
} status_bar_event_type;

typedef struct {
    status_bar_event_type type;
    uint8_t value;
} status_bar_event_t;

/**
 * @brief  Create status bar object
 *
 * @param[in]  cont  Pointer to parent container
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  status bar instance
 */
status_bar_t *status_bar_create(lv_obj_t *cont);
/**
 * @brief  Update status bar object
 *
 * @param[in]  sb  Pointer to status bar object
 * @param[in]  ev  Event for status bar
 */
void status_bar_update_state(status_bar_t *sb, status_bar_event_t ev);

#endif
