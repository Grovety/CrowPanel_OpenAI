/* WIFI window header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _WIFI_WINDOW_H_
#define _WIFI_WINDOW_H_

#include "common.h"

typedef struct {
    area_t ssid;
    area_t password;
    button_t connect;
    button_t disconnect;
    button_t scan;
} wifi_window_t;

typedef struct {
    char ssid[32];
    char password[64];
} selected_network_info_t;

/**
 * @brief  Create wifi window
 *
 * @param[in]  cont      Parent container
 * @param[in]  keyboard  Pointer to keyboard object
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  wifi window instance
 */
wifi_window_t *wifi_window_create(lv_obj_t *cont, lv_obj_t *keyboard);
/**
 * @brief  Update window depending on connection state
 *
 * @param[in]  win        Pointer to keyboard object
 * @param[in]  connected  Connected state (1, 0)
 */
void wifi_window_update(wifi_window_t *win, uint8_t connected);
/**
 * @brief  Add options to wifi SSID dropdown
 *
 * @param[in]  win     Pointer to keyboard object
 * @param[in]  buffer  Buffer of null terminated SSID strings
 * @param[in]  num     Number of null terminated SSID strings in buffer
 */
void add_wifi_options(wifi_window_t *win, const char *buffer, size_t num);
/**
 * @brief  Set password to text area
 *
 * @param[in]  win       Pointer to keyboard object
 * @param[in]  password  Password c-string
 */
void set_password(wifi_window_t *win, const char *password);

#endif
