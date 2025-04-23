#include "terminal.h"
#include "app.h"
#include "common.h"

#include "esp_heap_caps.h"
#include "misc/lv_anim.h"
#include "misc/lv_color.h"
#include "stdio.h"
#include "string.h"

#include "core/lv_obj_pos.h"
#include "font/lv_symbol_def.h"
#include "misc/lv_area.h"
#include "widgets/lv_label.h"

LV_FONT_DECLARE(icons)

#define SYMBOL_MIC_ON  "\xEF\x84\xB0"
#define SYMBOL_MIC_OFF "\xEF\x84\xB1"

#define PLACEHOLDER_TEXT "Toggle the mic on, speak your request, then toggle it off to send."

#define LOG_BUFFER_SIZE 2048

typedef struct {
    char *output_buffer;
    size_t output_buffer_len;
    size_t output_buffer_size;
} _terminal_private_t;

static void clr_btn_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    terminal_t *term = (terminal_t *)lv_event_get_user_data(e);
    switch (code) {
    case LV_EVENT_CLICKED:
        terminal_input_clear(term);
        terminal_output_clear(term);
        break;
    default:
        break;
    }
}

static void disconnect_btn_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        AppEvent_t ev = {.id = CHAT_SESSION_STOP, .data = NULL};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

static void mic_btn_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *icon = lv_event_get_user_data(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        bool enabled = lv_obj_get_state(btn) & LV_STATE_CHECKED;
        if (enabled) {
            lv_label_set_text(icon, SYMBOL_MIC_OFF);
            AppEvent_t ev = {.id = MIC_OFF, .data = NULL};
            xQueueSend(app_events_queue, &ev, 0);
        } else {
            lv_label_set_text(icon, SYMBOL_MIC_ON);
            AppEvent_t ev = {.id = MIC_ON, .data = NULL};
            xQueueSend(app_events_queue, &ev, 0);
        }
    }
}

lv_obj_t *create_buttons_area(terminal_t *term) {
    lv_obj_t *buttons_area = lv_obj_create(term->cont);
    lv_obj_set_size(buttons_area, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(buttons_area, &transparent_area_style, 0);
    lv_obj_set_style_pad_all(buttons_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(buttons_area, 5, LV_STATE_DEFAULT);

    term->clr.btn = lv_btn_create(buttons_area);
    lv_obj_set_size(term->clr.btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(term->clr.btn, 90, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(term->clr.btn, clr_btn_event_handler, LV_EVENT_CLICKED,
                        term);
    term->clr.label = lv_label_create(term->clr.btn);
    lv_label_set_text(term->clr.label, LV_SYMBOL_TRASH);

    term->disconnect.btn = lv_btn_create(buttons_area);
    lv_obj_set_size(term->disconnect.btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(term->disconnect.btn, 90, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(term->disconnect.btn, disconnect_btn_event_handler,
                        LV_EVENT_CLICKED, term);
    term->disconnect.label = lv_label_create(term->disconnect.btn);
    lv_label_set_text(term->disconnect.label, "Disconnect");

    term->mic.btn = lv_btn_create(buttons_area);
    lv_obj_set_style_pad_all(term->mic.btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(term->mic.btn, 90, LV_STATE_DEFAULT);
    lv_obj_add_flag(term->mic.btn, LV_OBJ_FLAG_CHECKABLE);

    term->mic.label = lv_label_create(term->mic.btn);
    lv_obj_set_style_text_font(term->mic.label, &icons, 0);
    lv_obj_align(term->mic.label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(term->mic.label, SYMBOL_MIC_OFF);

    lv_obj_add_event_cb(term->mic.btn, mic_btn_event_handler, LV_EVENT_ALL,
                        term->mic.label);
    lv_obj_add_state(term->mic.btn, LV_STATE_CHECKED);

    static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT,
                                        LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(buttons_area, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(term->clr.btn, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(term->disconnect.btn, LV_GRID_ALIGN_CENTER, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(term->mic.btn, LV_GRID_ALIGN_START, 2, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_set_height(term->mic.btn, lv_pct(100));
    lv_obj_update_layout(term->mic.btn);
    lv_obj_set_width(term->mic.btn, lv_pct(30));

    return buttons_area;
}

terminal_t *terminal_create(lv_obj_t *cont) {
    terminal_t *term = (terminal_t *)malloc(sizeof(terminal_t));
    _terminal_private_t *priv =
        (_terminal_private_t *)malloc(sizeof(_terminal_private_t));
    priv->output_buffer_size = LOG_BUFFER_SIZE;
    priv->output_buffer_len = 0;
    priv->output_buffer =
        heap_caps_calloc(priv->output_buffer_size + 1, 1,
                         MALLOC_CAP_SPIRAM); // with null terminator
    term->_private = priv;

    term->cont = lv_obj_create(cont);
    lv_obj_set_size(term->cont, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(term->cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(term->cont, &transparent_area_style, 0);
    lv_obj_set_style_border_width(term->cont, 0, LV_STATE_DEFAULT);

    lv_obj_set_style_pad_all(term->cont, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(term->cont, 5, LV_STATE_DEFAULT);

    lv_obj_t *input_area = lv_obj_create(term->cont);
    lv_obj_clear_flag(input_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(input_area, lv_pct(100), lv_pct(10));
    lv_obj_set_style_pad_all(input_area, 5, LV_STATE_DEFAULT);

    term->input = lv_label_create(input_area);
    lv_obj_set_height(term->input, LV_SIZE_CONTENT);
    lv_label_set_long_mode(term->input, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(term->input, &S_FONT, 0);
    lv_label_set_text(term->input, "");

    lv_obj_t *output_area = lv_obj_create(term->cont);
    lv_obj_set_size(output_area, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(output_area, 5, LV_STATE_DEFAULT);

    term->output = lv_label_create(output_area);
    lv_obj_set_size(term->output, lv_pct(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(term->output, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(term->output, &S_FONT, 0);
    lv_label_set_text_static(term->output, PLACEHOLDER_TEXT);

    lv_obj_t *buttons_area = create_buttons_area(term);

    static lv_coord_t grid_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1),
                                        LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(term->cont, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(input_area, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_START, 0, 1);
    lv_obj_set_grid_cell(output_area, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_grid_cell(buttons_area, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_START, 2, 1);

    return term;
}

void terminal_output_clear(terminal_t *term) {
    _terminal_private_t *priv = (_terminal_private_t *)term->_private;
    memset(priv->output_buffer, 0, priv->output_buffer_size);
    priv->output_buffer_len = 0;
    lv_label_set_text_static(term->output, PLACEHOLDER_TEXT);
    lv_obj_scroll_to(lv_obj_get_parent(term->output), 0, 0, LV_ANIM_OFF);
}

void terminal_input_clear(terminal_t *term) {
    lv_label_set_text(term->input, "");
}

void terminal_output_add(terminal_t *term, const char *buffer, size_t len) {
    if (term == NULL || buffer == NULL || term->_private == NULL) {
        return;
    }
    _terminal_private_t *priv = (_terminal_private_t *)term->_private;

    if (len > 0) {
        // Truncate input if larger than buffer size
        if (len > priv->output_buffer_size) {
            len = priv->output_buffer_size;
            priv->output_buffer_len = 0;
        }

        while (priv->output_buffer_len + len > priv->output_buffer_size) {
            // Remove at least one complete line from the beginning.
            // Find the first non-newline character starting from the
            // beginning.
            const char *start_pos = priv->output_buffer;
            while (*start_pos == '\n' && *start_pos != '\0') {
                start_pos++;
            }
            // Look for the next newline from the first non-newline
            // character.
            const char *newline_pos = strchr(start_pos, '\n');
            if (newline_pos) {
                // Advance past consecutive newline characters.
                while (*newline_pos == '\n')
                    newline_pos++;
                size_t chars_to_remove = newline_pos - priv->output_buffer;
                if (chars_to_remove > priv->output_buffer_len) {
                    chars_to_remove = priv->output_buffer_len;
                }
                // Shift the remaining buffer content to the beginning.
                priv->output_buffer_len -= chars_to_remove;
                memmove(priv->output_buffer, newline_pos,
                        priv->output_buffer_len);
            } else {
                // If no newline is found, remove the first half of the
                // current buffer.
                size_t half_len = priv->output_buffer_len / 2;
                memmove(priv->output_buffer, priv->output_buffer + half_len,
                        priv->output_buffer_len - half_len);
                priv->output_buffer_len -= half_len;
            }
        }
        // Append new content into the buffer.
        memcpy(priv->output_buffer + priv->output_buffer_len, buffer, len);
        priv->output_buffer_len += len;
        // Clear the unused portion of the buffer to avoid stale data.
        memset(priv->output_buffer + priv->output_buffer_len, 0,
               priv->output_buffer_size - priv->output_buffer_len);
    }
    // Update the output widget.
    lv_label_set_text_static(term->output, priv->output_buffer);
}

void terminal_transcript_add(terminal_t *term, const char *buffer, size_t len) {
    lv_label_set_text(term->input, buffer);
}
