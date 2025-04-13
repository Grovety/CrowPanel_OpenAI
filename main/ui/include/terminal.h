/* Terminal window header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *cont;
    lv_obj_t *input;
    lv_obj_t *output;
    lv_obj_t *icon;
    button_t clr;
    button_t disconnect;
    void *_private;
} terminal_t;

typedef enum {
    TERM_ICON_READY = 0,
    TERM_ICON_PROCESSING = 1,
} terminal_icon_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Create terminal object
 *
 * @param[in]  cont                 Pointer to parent container
 * @param[in]  disconnect_event_cb  Callback for `Disconnected` button
 *
 * @return
 *       - NULL    Failed to create
 *       - Others  terminal instance
 */
terminal_t *terminal_create(lv_obj_t *cont, lv_event_cb_t disconnect_event_cb);
/**
 * @brief Add data to the terminal output area
 *
 * @param[in] term    Pointer to terminal object
 * @param[in] buffer  Character buffer to add to the terminal
 * @param[in] len     Buffer len
 */
void terminal_output_add(terminal_t *term, const char *buffer, size_t len);
/**
 * @brief Add data to the terminal transcript area
 *
 * @param[in] term    Pointer to terminal object
 * @param[in] buffer  Character buffer to add to the terminal
 * @param[in] len     Buffer len
 */
void terminal_transcript_add(terminal_t *term, const char *buffer, size_t len);
/**
 * @brief Clear terminal output area
 *
 * @param[in] term  Pointer to terminal object
 */
void terminal_output_clear(terminal_t *term);
/**
 * @brief Update terminal status icon
 *
 * @param[in] term  Pointer to terminal object
 * @param[in] state  New state
 */
void terminal_update_icon(terminal_t *term, terminal_icon_state_t state);

#ifdef __cplusplus
}
#endif

#endif // TERMINAL_H
