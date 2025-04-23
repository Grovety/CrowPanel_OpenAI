#include "auth_window.h"
#include "app.h"

#include "esp_log.h"

static const char *TAG = "auth_window";

static void connect_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED) {
        char *buffer = NULL;
        const char *api_key_view = lv_textarea_get_text(ta);
        int len = strlen(api_key_view);
        if (len) {
            buffer = (char *)malloc(len + 1);
            strncpy(buffer, api_key_view, len);
            buffer[len] = '\0';
        } else {
            ESP_LOGW(TAG, "no API key provided");
        }
        AppEvent_t ev = {.id = CHAT_SESSION_START, .data = buffer};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

auth_window_t *auth_window_create(lv_obj_t *cont, lv_obj_t *keyboard) {
    auth_window_t *win = (auth_window_t *)malloc(sizeof(auth_window_t));

    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_all(cont, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 10, LV_STATE_DEFAULT);

    win->api_key.cont = lv_obj_create(cont);
    lv_obj_set_flex_align(win->api_key.cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(win->api_key.cont, lv_pct(100), lv_pct(50));
    lv_obj_set_flex_flow(win->api_key.cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(win->api_key.cont, 10, LV_STATE_DEFAULT);
    win->api_key.label = lv_label_create(win->api_key.cont);
    lv_label_set_text(win->api_key.label, "API key");
    lv_obj_set_style_text_font(win->api_key.label, &M_FONT, 0);
    win->api_key.field = lv_textarea_create(win->api_key.cont);
    lv_textarea_set_text(win->api_key.field, "");
    lv_obj_clear_flag(win->api_key.field, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(win->api_key.field, 1);
    lv_obj_set_size(win->api_key.field, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(win->api_key.field, text_area_event_cb, LV_EVENT_ALL,
                        keyboard);
    lv_obj_set_style_text_font(win->api_key.field, &S_FONT, 0);

    lv_obj_t *buttons_area = lv_obj_create(cont);
    lv_obj_set_style_pad_all(buttons_area, 10, LV_STATE_DEFAULT);
    lv_obj_set_flex_align(buttons_area, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(buttons_area, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(buttons_area, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(buttons_area, &transparent_area_style, 0);

    win->connect.btn = lv_btn_create(buttons_area);
    win->connect.label = lv_label_create(win->connect.btn);
    lv_label_set_text(win->connect.label, "Connect");
    lv_obj_add_event_cb(win->connect.btn, connect_btn_event_cb,
                        LV_EVENT_CLICKED, win->api_key.field);
    return win;
}

void set_api_key(auth_window_t *win, const char *api_key) {
    lv_textarea_set_text(win->api_key.field, api_key);
}
