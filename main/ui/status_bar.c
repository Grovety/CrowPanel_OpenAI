#include "status_bar.h"
#include "common.h"

#include "font/lv_symbol_def.h"

#include "esp_log.h"

status_bar_t *status_bar_create(lv_obj_t *cont) {
    status_bar_t *sb = (status_bar_t *)malloc(sizeof(status_bar_t));

    sb->bar = lv_obj_create(cont);
    lv_obj_set_size(sb->bar, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_clear_flag(sb->bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_all(sb->bar, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sb->bar, 0, LV_STATE_DEFAULT);

    lv_obj_set_layout(sb->bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sb->bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sb->bar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_add_style(sb->bar, &m_font_style, 0);

    sb->webrtc = lv_label_create(sb->bar);
    lv_label_set_text(sb->webrtc, "");
    sb->wifi = lv_label_create(sb->bar);
    lv_label_set_text(sb->wifi, "");

    sb->separator = lv_obj_create(cont);
    lv_obj_set_size(sb->separator, LV_PCT(100), 3);
    lv_obj_set_style_bg_color(sb->separator, lv_color_hex(0x007BFF),
                              0); // Blue color
    lv_obj_set_style_radius(sb->separator, 0, 0);
    lv_obj_set_style_border_width(sb->separator, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(sb->separator, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sb->separator, 0, LV_STATE_DEFAULT);
    return sb;
}

void status_bar_update_state(status_bar_t *sb, status_bar_event_t ev) {
    ESP_LOGI(__FUNCTION__, "ev: type=%d, value=%d", ev.type, ev.value);
    if (sb) {
        switch (ev.type) {
        case SB_WIFI:
            if (ev.value) {
                lv_label_set_text(sb->wifi, LV_SYMBOL_WIFI);
            } else {
                lv_label_set_text(sb->wifi, "");
            }
            break;
        case SB_WEBRTC:
            if (ev.value) {
                lv_label_set_text(sb->webrtc, LV_SYMBOL_OK);
            } else {
                lv_label_set_text(sb->webrtc, "");
            }
            break;
        default:
            break;
        }
    }
}
