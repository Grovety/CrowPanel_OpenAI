#include "ui.h"
#include "app.h"
#include "settings_window.h"

#include "font/lv_symbol_def.h"

#include "esp_log.h"

static const char *TAG = "ui";

static void api_disconnect_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    terminal_t *term = (terminal_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED) {
        terminal_output_clear(term);
        AppEvent_t ev = {.id = WEBRTC_STOP, .data = NULL};
        xQueueSend(app_events_queue, &ev, 0);
    }
}

config_screen_t *config_screen_create(lv_obj_t *cont, lv_obj_t *keyboard) {
    config_screen_t *src = (config_screen_t *)malloc(sizeof(config_screen_t));

    src->tabview = lv_tabview_create(cont, LV_DIR_LEFT, 80);
    lv_obj_set_size(src->tabview, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(src->tabview, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(src->tabview, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(src->tabview, 0, LV_STATE_DEFAULT);
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(src->tabview);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_RIGHT,
                                 LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_t *wifi_tab = lv_tabview_add_tab(src->tabview, "WIFI");
    lv_obj_t *auth_tab = lv_tabview_add_tab(src->tabview, "AUTH");
    lv_obj_t *settings_tab =
        lv_tabview_add_tab(src->tabview, LV_SYMBOL_SETTINGS);

    lv_obj_clear_flag(lv_tabview_get_content(src->tabview),
                      LV_OBJ_FLAG_SCROLLABLE);

    src->wifi_win = wifi_window_create(wifi_tab, keyboard);
    src->auth_win = auth_window_create(auth_tab, keyboard);
    src->settings_win = settings_window_create(settings_tab, NULL);
    return src;
}

main_cont_t *ui_create() {
    ESP_LOGI(TAG, "Creating UI");
    ui_styles_init();

    main_cont_t *main_cont = (main_cont_t *)malloc(sizeof(main_cont_t));

    main_cont->processing_popup = processing_popup_create(lv_layer_top());

    main_cont->keyboard = lv_keyboard_create(lv_layer_top());
    lv_keyboard_set_textarea(main_cont->keyboard, NULL);
    lv_obj_add_flag(main_cont->keyboard, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(root, &transparent_area_style, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(root, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(root, 0, LV_STATE_DEFAULT);

    main_cont->status_bar = status_bar_create(root);

    main_cont->content = lv_obj_create(root);
    lv_obj_set_width(main_cont->content, lv_pct(100));
    lv_obj_set_flex_grow(main_cont->content, 1);
    lv_obj_clear_flag(main_cont->content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(main_cont->content, &transparent_area_style, 0);
    lv_obj_set_layout(main_cont->content, LV_LAYOUT_FLEX);
    lv_obj_set_style_pad_all(main_cont->content, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(main_cont->content, 0, LV_STATE_DEFAULT);

    main_cont->config_screen =
        config_screen_create(main_cont->content, main_cont->keyboard);

    main_cont->terminal =
        terminal_create(main_cont->content, api_disconnect_btn_event_cb);
    switch_screen(main_cont, CONFIG_SCREEN);
    return main_cont;
}

void switch_screen(main_cont_t *main_cont, screen_id id) {
    switch (id) {
    case CONFIG_SCREEN:
        lv_obj_add_flag(main_cont->terminal->cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(main_cont->config_screen->tabview,
                          LV_OBJ_FLAG_HIDDEN);
        break;
    case TERMINAL_SCREEN:
        lv_obj_add_flag(main_cont->config_screen->tabview, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(main_cont->terminal->cont, LV_OBJ_FLAG_HIDDEN);
        break;
    default:
        break;
    }
}
