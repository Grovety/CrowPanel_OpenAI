#include "terminal.h"
#include "common.h"

#include "stdio.h"
#include "string.h"

#include "core/lv_obj_pos.h"
#include "font/lv_symbol_def.h"
#include "misc/lv_area.h"
#include "widgets/lv_label.h"

LV_FONT_DECLARE(icons)

#define SYMBOL_MIC   "\xEF\x84\xB0"
#define SYMBOL_ARROW "\xEF\x81\xA1"
#define SYMBOL_PAUSE "\xEF\x81\x8D"

#define LOG_BUFFER_SIZE 1024

typedef struct {
    char *output_buffer;
    size_t output_buffer_len;
    size_t output_buffer_size;

    terminal_icon_state_t prev_icon_state;
} _terminal_private_t;

static void clr_click_action(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    terminal_t *term = (terminal_t *)lv_event_get_user_data(e);
    switch (code) {
    case LV_EVENT_CLICKED:
        terminal_output_clear(term);
        break;
    default:
        break;
    }
}

lv_obj_t *create_input_area(terminal_t *term) {
    lv_obj_t *input_area = lv_obj_create(term->cont);
    lv_obj_clear_flag(input_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(input_area, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(input_area, 5, LV_STATE_DEFAULT);

    term->input = lv_label_create(input_area);
    lv_obj_set_height(term->input, LV_SIZE_CONTENT);
    lv_label_set_long_mode(term->input, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(term->input, &s_font_style, 0);
    lv_label_set_text(term->input, "");

    lv_obj_t *status_area = lv_obj_create(input_area);
    lv_obj_set_size(status_area, 32, 32);
    lv_obj_set_style_pad_all(status_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(status_area, 90, LV_STATE_DEFAULT);
    lv_obj_clear_flag(status_area, LV_OBJ_FLAG_SCROLLABLE);

    lv_color_t border_color =
        lv_obj_get_style_border_color(status_area, LV_PART_MAIN);

    term->icon = lv_label_create(status_area);
    lv_label_set_text(term->icon, SYMBOL_MIC);
    lv_obj_set_style_text_font(term->icon, &icons, 0);
    lv_obj_set_style_text_color(term->icon, border_color, 0);
    lv_obj_align(term->icon, LV_ALIGN_CENTER, 0, 0);

    static lv_coord_t grid_col_dsc[] = {LV_GRID_FR(1), LV_GRID_CONTENT,
                                        LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(input_area, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(term->input, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(status_area, LV_GRID_ALIGN_END, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    return input_area;
}

terminal_t *terminal_create(lv_obj_t *cont, lv_event_cb_t disconnect_event_cb) {
    terminal_t *term = (terminal_t *)malloc(sizeof(terminal_t));
    _terminal_private_t *priv =
        (_terminal_private_t *)malloc(sizeof(_terminal_private_t));
    priv->output_buffer_size = LOG_BUFFER_SIZE;
    priv->output_buffer_len = 0;
    priv->output_buffer =
        calloc(priv->output_buffer_size + 1, 1); // with null terminator
    term->_private = priv;

    term->cont = lv_obj_create(cont);
    lv_obj_set_size(term->cont, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(term->cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(term->cont, &transparent_area_style, 0);
    lv_obj_set_style_border_width(term->cont, 0, LV_STATE_DEFAULT);

    lv_obj_set_style_pad_all(term->cont, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(term->cont, 5, LV_STATE_DEFAULT);

    lv_obj_t *input_area = create_input_area(term);

    lv_obj_t *output_area = lv_obj_create(term->cont);
    lv_obj_clear_flag(output_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(output_area, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(output_area, 5, LV_STATE_DEFAULT);

    term->output = lv_label_create(output_area);
    lv_obj_set_size(term->output, lv_pct(100), lv_pct(100));
    lv_label_set_long_mode(term->output, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(term->output, &s_font_style, 0);
    lv_label_set_text_static(term->output, priv->output_buffer);

    lv_obj_t *buttons_area = lv_obj_create(term->cont);
    lv_obj_set_height(buttons_area, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(buttons_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(buttons_area, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(buttons_area, &transparent_area_style, 0);
    lv_obj_clear_flag(buttons_area, LV_OBJ_FLAG_SCROLLABLE);

    term->clr.btn = lv_btn_create(buttons_area);
    lv_obj_add_event_cb(term->clr.btn, clr_click_action, LV_EVENT_CLICKED,
                        term);
    term->clr.label = lv_label_create(term->clr.btn);
    lv_label_set_text(term->clr.label, LV_SYMBOL_TRASH);

    term->disconnect.btn = lv_btn_create(buttons_area);
    term->disconnect.label = lv_label_create(term->disconnect.btn);
    lv_obj_add_event_cb(term->disconnect.btn, disconnect_event_cb,
                        LV_EVENT_CLICKED, term);
    lv_label_set_text(term->disconnect.label, "Disconnect");

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

    terminal_update_icon(term, TERM_ICON_READY);

    return term;
}

void terminal_output_clear(terminal_t *term) {
    _terminal_private_t *priv = (_terminal_private_t *)term->_private;
    memset(priv->output_buffer, 0, priv->output_buffer_size);
    priv->output_buffer_len = 0;
    lv_obj_invalidate(term->output);
    lv_label_set_text(term->input, "");
}

void terminal_output_add(terminal_t *term, const char *buffer, size_t len) {
    if (term == NULL || buffer == NULL || term->_private == NULL) {
        return;
    }
    _terminal_private_t *priv = (_terminal_private_t *)term->_private;

    lv_style_value_t v;
    lv_style_get_prop(&s_font_style, LV_STYLE_TEXT_FONT, &v);

    lv_coord_t window_width = lv_obj_get_width(term->output);
    lv_coord_t window_height = lv_obj_get_height(term->output);

    if (len > 0) {
        // Truncate input if larger than buffer size
        if (len > priv->output_buffer_size) {
            len = priv->output_buffer_size;
            priv->output_buffer_len = 0;
        }

        bool need_space = false;
        do {
            if (priv->output_buffer_len + len > priv->output_buffer_size ||
                need_space) {
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
                // After removal, reset the flag so that we try appending again.
                need_space = false;
            }
            // Append new content into the buffer.
            memcpy(priv->output_buffer + priv->output_buffer_len, buffer, len);
            priv->output_buffer_len += len;
            // Clear the unused portion of the buffer to avoid stale data.
            memset(priv->output_buffer + priv->output_buffer_len, 0,
                   priv->output_buffer_size - priv->output_buffer_len);

            // Compute the rendered text size to check if it fits in the window.
            lv_point_t text_size;
            lv_txt_get_size(&text_size, priv->output_buffer, v.ptr, 0, 0,
                            window_width, LV_TEXT_FLAG_NONE);

            // If text height exceeds the window height, rollback the new
            // addition and mark for removal.
            if (text_size.y > window_height) {
                need_space = true;
                // Rollback the newly appended text.
                priv->output_buffer_len -= len;
                priv->output_buffer[priv->output_buffer_len] = '\0';
            }
        } while (need_space);

        // Trigger a redraw or update for the output widget.
        lv_obj_invalidate(term->output);
    }
}

void terminal_transcript_add(terminal_t *term, const char *buffer, size_t len) {
    lv_label_set_text(term->input, buffer);
}

void terminal_update_icon(terminal_t *term, terminal_icon_state_t state) {
    _terminal_private_t *priv = (_terminal_private_t *)term->_private;
    if (state != priv->prev_icon_state) {
        switch (state) {
        default:
        case TERM_ICON_READY:
            lv_label_set_text(term->icon, SYMBOL_MIC);
            break;
        case TERM_ICON_PROCESSING:
            lv_label_set_text(term->icon, SYMBOL_PAUSE);
            break;
        }
        priv->prev_icon_state = state;
    }
}