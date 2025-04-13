#include "common.h"

LV_FONT_DECLARE(noto_sans_18)

lv_style_t m_font_style;
lv_style_t s_font_style;
lv_style_t transparent_area_style;

void ui_styles_init() {
    lv_style_init(&m_font_style);
    lv_style_set_text_font(&m_font_style, &lv_font_montserrat_26);

    lv_style_init(&s_font_style);
    lv_style_set_text_font(&s_font_style, &noto_sans_18);

    lv_style_init(&transparent_area_style);
    lv_style_set_border_opa(&transparent_area_style, 0);
    lv_style_set_bg_opa(&transparent_area_style, 0);
}

void text_area_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    switch (code) {
    case LV_EVENT_PRESSED:
        if (!lv_keyboard_get_textarea(kb) &&
            (lv_obj_get_state(ta) & LV_STATE_FOCUSED)) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case LV_EVENT_FOCUSED:
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        break;
    case LV_EVENT_DEFOCUSED:
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        break;
    case LV_EVENT_READY:
        LV_LOG_TRACE("Ready, current text: %s", lv_textarea_get_text(ta));
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    default:
        break;
    }
}
