#include "wifi_window.h"
#include "app.h"
#include "common.h"

#include "widgets/lv_dropdown.h"
#include "widgets/lv_textarea.h"

#include "esp_log.h"

static const char *TAG = "wifi_window";

static lv_style_t connect_btn_style;
static lv_style_t disconnect_btn_style;

static void dropdown_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_dropdown_set_text(dropdown, NULL);
    }
}

static void wifi_connect_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    wifi_window_t *win = (wifi_window_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED) {
        selected_network_info_t *s =
            (selected_network_info_t *)malloc(sizeof(selected_network_info_t));
        memset(s, 0, sizeof(selected_network_info_t));
        lv_dropdown_get_selected_str(win->ssid.field, s->ssid, sizeof(s->ssid));
        const char *password = lv_textarea_get_text(win->password.field);
        strncpy(s->password, password, sizeof(s->password));
        AppEvent_t ev = {.id = WIFI_CONNECT, .data = s};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

static void wifi_disconnect_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        AppEvent_t ev = {.id = WIFI_DISCONNECT, .data = NULL};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

static void wifi_scan_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        AppEvent_t ev = {.id = WIFI_START_SCAN, .data = NULL};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

wifi_window_t *wifi_window_create(lv_obj_t *cont, lv_obj_t *keyboard) {
    wifi_window_t *win = (wifi_window_t *)malloc(sizeof(wifi_window_t));
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_all(cont, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 10, LV_STATE_DEFAULT);

    win->ssid.cont = lv_obj_create(cont);
    lv_obj_set_style_pad_all(win->ssid.cont, 10, LV_STATE_DEFAULT);
    lv_obj_set_flex_align(win->ssid.cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(win->ssid.cont);
    lv_obj_set_size(win->ssid.cont, lv_pct(80), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(win->ssid.cont, LV_FLEX_FLOW_ROW);
    win->ssid.label = lv_label_create(win->ssid.cont);
    lv_label_set_text(win->ssid.label, "SSID");
    lv_obj_set_style_text_font(win->ssid.label, &M_FONT, 0);
    lv_obj_set_flex_grow(win->ssid.label, 1);
    win->ssid.field = lv_dropdown_create(win->ssid.cont);
    lv_dropdown_clear_options(win->ssid.field);
    lv_obj_add_event_cb(win->ssid.field, dropdown_event_handler, LV_EVENT_ALL,
                        NULL);
    lv_obj_set_flex_grow(win->ssid.field, 2);
    lv_obj_set_style_text_font(win->ssid.field, &M_FONT, 0);

    win->password.cont = lv_obj_create(cont);
    lv_obj_set_style_pad_all(win->password.cont, 10, LV_STATE_DEFAULT);
    lv_obj_set_flex_align(win->password.cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(win->password.cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(win->password.cont, lv_pct(80), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(win->password.cont, LV_FLEX_FLOW_ROW);
    win->password.label = lv_label_create(win->password.cont);
    lv_label_set_text(win->password.label, "Password");
    lv_obj_set_style_text_font(win->password.label, &M_FONT, 0);
    lv_obj_set_flex_grow(win->password.label, 1);
    win->password.field = lv_textarea_create(win->password.cont);
    lv_textarea_set_one_line(win->password.field, 1);
    lv_obj_add_event_cb(win->password.field, text_area_event_cb, LV_EVENT_ALL,
                        keyboard);
    lv_textarea_set_placeholder_text(win->password.field, "Enter Password");
    lv_obj_set_flex_grow(win->password.field, 2);
    lv_obj_set_style_text_font(win->password.field, &M_FONT, 0);

    lv_style_init(&connect_btn_style);
    lv_style_set_bg_color(&connect_btn_style, lv_palette_main(LV_PALETTE_BLUE));

    lv_style_init(&disconnect_btn_style);
    lv_style_set_bg_color(&disconnect_btn_style,
                          lv_palette_main(LV_PALETTE_RED));

    lv_obj_t *buttons_area = lv_obj_create(cont);
    lv_obj_set_style_pad_all(buttons_area, 10, LV_STATE_DEFAULT);
    lv_obj_set_size(buttons_area, lv_pct(80), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(buttons_area, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(buttons_area, &transparent_area_style, 0);
    lv_obj_set_flex_align(buttons_area, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win->connect.btn = lv_btn_create(buttons_area);
    lv_obj_add_style(win->connect.btn, &connect_btn_style, 0);
    lv_obj_set_flex_grow(win->connect.btn, 1);
    win->connect.label = lv_label_create(win->connect.btn);
    lv_label_set_text(win->connect.label, "Connect");
    lv_obj_center(win->connect.label);
    lv_obj_add_event_cb(win->connect.btn, wifi_connect_btn_event_cb,
                        LV_EVENT_CLICKED, win);

    win->scan.btn = lv_btn_create(buttons_area);
    lv_obj_set_flex_grow(win->scan.btn, 1);
    win->scan.label = lv_label_create(win->scan.btn);
    lv_label_set_text(win->scan.label, "Scan");
    lv_obj_center(win->scan.label);
    lv_obj_add_event_cb(win->scan.btn, wifi_scan_btn_event_cb, LV_EVENT_CLICKED,
                        NULL);

    win->disconnect.btn = lv_btn_create(buttons_area);
    win->disconnect.label = lv_label_create(win->disconnect.btn);
    lv_label_set_text(win->disconnect.label, "Disconnect");
    lv_obj_center(win->disconnect.label);
    lv_obj_add_event_cb(win->disconnect.btn, wifi_disconnect_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(win->disconnect.btn, &disconnect_btn_style, 0);
    lv_obj_add_flag(win->disconnect.btn, LV_OBJ_FLAG_HIDDEN);
    return win;
}

void wifi_window_update(wifi_window_t *win, uint8_t connected) {
    if (connected) {
        lv_obj_clear_flag(win->disconnect.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(win->connect.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(win->scan.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(win->ssid.field, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(win->password.field, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_add_flag(win->disconnect.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(win->connect.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(win->scan.btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(win->ssid.field, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(win->password.field, LV_OBJ_FLAG_CLICKABLE);
        lv_textarea_set_text(win->password.field, "");
    }
}

void add_wifi_options(wifi_window_t *win, const char *buffer, size_t num) {
    const char *ptr = buffer;
    for (int i = 0; i < num; i++) {
        ESP_LOGD(TAG, "SSID=%d: \"%s\"", i + 1, ptr);
        lv_dropdown_add_option(win->ssid.field, ptr, i);
        ptr += strlen(ptr) + 1;
    }
    lv_dropdown_set_text(win->ssid.field, NULL);
}

void set_password(wifi_window_t *win, const char *password) {
    lv_textarea_set_text(win->password.field, password);
}
