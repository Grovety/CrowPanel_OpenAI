#include "popup.h"
#include "common.h"

#include "extra/layouts/flex/lv_flex.h"
#include "misc/lv_area.h"

processing_popup_t *processing_popup_create(lv_obj_t *cont) {
    processing_popup_t *pp =
        (processing_popup_t *)malloc(sizeof(processing_popup_t));
    pp->window = lv_obj_create(cont);
    lv_obj_set_size(pp->window, 200, 200);
    lv_obj_center(pp->window);
    pp->spinner = lv_spinner_create(pp->window, 1000, 60);
    lv_obj_set_size(pp->spinner, 150, 150);
    lv_obj_center(pp->spinner);
    lv_obj_add_flag(pp->window, LV_OBJ_FLAG_HIDDEN);
    return pp;
}

void processing_popup_show(processing_popup_t *obj, uint8_t state) {
    processing_popup_t *pp = (processing_popup_t *)obj;
    if (state) {
        lv_obj_clear_flag(pp->window, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(pp->window, LV_OBJ_FLAG_HIDDEN);
    }
}

static void mbox_event_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_current_target(e);
    LV_LOG_TRACE("Button %s clicked", lv_msgbox_get_active_btn_text(obj));
    lv_msgbox_close(obj);
}

void msg_box_create(const char *message) {
    static const char *btns[] = {"Ok", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "", message, btns, false);
    lv_obj_add_event_cb(mbox, mbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
    lv_obj_set_width(mbox, lv_pct(60));
    lv_obj_t *msg = lv_msgbox_get_text(mbox);
    lv_obj_set_style_text_font(msg, &M_FONT, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *buttons = lv_msgbox_get_btns(mbox);
    lv_btnmatrix_set_btn_width(buttons, 0, 2);
    lv_obj_set_flex_align(mbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
}
