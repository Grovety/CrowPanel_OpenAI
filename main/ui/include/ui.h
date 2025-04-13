/* UI header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _UI_H_
#define _UI_H_

#include "auth_window.h"
#include "common.h"
#include "popup.h"
#include "settings_window.h"
#include "status_bar.h"
#include "terminal.h"
#include "wifi_window.h"

typedef struct {
    lv_obj_t *tabview;
    wifi_window_t *wifi_win;
    auth_window_t *auth_win;
    settings_window_t *settings_win;
} config_screen_t;

typedef enum {
    CONFIG_SCREEN,
    TERMINAL_SCREEN,
} screen_id;

typedef struct {
    status_bar_t *status_bar;
    lv_obj_t *keyboard;
    processing_popup_t *processing_popup;
    lv_obj_t *content;
    config_screen_t *config_screen;
    terminal_t *terminal;
} main_cont_t;

/**
 * @brief  Create main container
 *
 * @return
 *       - NULL    Fail to create
 *       - Others  main_cont_t pointer
 */
main_cont_t *ui_create();
/**
 * @brief  Switch visible screen
 *
 * @param[in]  main_cont  Pointer to main container
 * @param[in]  id         Screed identificator
 */
void switch_screen(main_cont_t *main_cont, screen_id id);

#endif
