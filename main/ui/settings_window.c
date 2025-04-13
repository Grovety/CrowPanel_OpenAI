#include "settings_window.h"
#include "app.h"

#include "esp_log.h"

static void clear_storage_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        AppEvent_t ev = {.id = STORAGE_CLEAR, .data = NULL};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

settings_window_t *settings_window_create(lv_obj_t *cont, lv_obj_t *keyboard) {
    settings_window_t *win =
        (settings_window_t *)malloc(sizeof(settings_window_t));
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_all(cont, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 10, LV_STATE_DEFAULT);

    lv_obj_t *buttons_area = lv_obj_create(cont);
    lv_obj_set_style_pad_all(buttons_area, 10, LV_STATE_DEFAULT);
    lv_obj_set_size(buttons_area, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(buttons_area, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(buttons_area, &transparent_area_style, 0);
    lv_obj_set_flex_align(buttons_area, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win->clear_storage.btn = lv_btn_create(buttons_area);
    win->clear_storage.label = lv_label_create(win->clear_storage.btn);
    lv_label_set_text(win->clear_storage.label, "Clear storage");
    lv_obj_center(win->clear_storage.label);
    lv_obj_add_event_cb(win->clear_storage.btn, clear_storage_btn_event_cb,
                        LV_EVENT_CLICKED, win);
    return win;
}
