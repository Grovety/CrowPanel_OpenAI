#include "popup.h"
#include "status_bar.h"
#include "terminal.h"
#include "ui.h"
#include "wifi_window.h"

#include "app.h"
#include "board.h"
#include "media_sys.h"
#include "network.h"
#include "nvs.h"
#include "openai.h"
#include "openai_webrtc.h"
#include "wifi.h"

#include "media_lib_adapter.h"
#include "media_lib_os.h"

#include "esp_log.h"

extern bool lvgl_port_lock(int timeout_ms);
extern bool lvgl_port_unlock(void);

#define LVGL_MUTEX_LOCKED_CALL(func)                                           \
    do {                                                                       \
        lvgl_port_lock(-1);                                                    \
        func;                                                                  \
        lvgl_port_unlock();                                                    \
    } while (false)

#define APP_SETTINGS_PART_NAME "app-setings"
#define WIFI_SSID_KEY          "ssid"
#define WIFI_PSW_KEY           "psw"
#define API_KEY_KEY            "api_key"

static const char *TAG = "app";

EventGroupHandle_t app_status_bits = NULL;
QueueHandle_t app_events_queue = NULL;


struct {
    selected_network_info_t network_info;
    openai_ctx_t *openai_ctx;
    main_cont_t *main_cont;
} static s_app_ctx;

static void app_events_task(void *pv) {
    for (;;) {
        AppEvent_t ev;
        xQueueReceive(app_events_queue, &ev, portMAX_DELAY);
        switch (ev.id) {
        case WIFI_START_SCAN:
            ESP_LOGI(TAG, "WIFI_START_SCAN");
            LVGL_MUTEX_LOCKED_CALL(
                processing_popup_show(s_app_ctx.main_cont->processing_popup, 1));
            wifi_start_scan();
            break;
        case WIFI_CONNECT: {
            ESP_LOGI(TAG, "WIFI_CONNECT");
            selected_network_info_t *con = (selected_network_info_t *)ev.data;
            if (con) {
                memcpy(&s_app_ctx.network_info, con,
                       sizeof(selected_network_info_t));
                ESP_LOGI(TAG, "Try to connect to SSID: %s, password: %s",
                         s_app_ctx.network_info.ssid,
                         s_app_ctx.network_info.password);
                if (s_app_ctx.network_info.ssid[0] == '\0') {
                    ESP_LOGW(TAG, "Provide SSID");
                    LVGL_MUTEX_LOCKED_CALL(msg_box_create("Provide SSID"));
                } else if (s_app_ctx.network_info.password[0] == '\0') {
                    ESP_LOGW(TAG, "Provide password");
                    LVGL_MUTEX_LOCKED_CALL(msg_box_create("Provide password"));
                } else {
                    LVGL_MUTEX_LOCKED_CALL(
                        processing_popup_show(s_app_ctx.main_cont->processing_popup, 1));
                    wifi_connect(s_app_ctx.network_info.ssid,
                                 s_app_ctx.network_info.password);
                }
                free(con);
            } else {
                ESP_LOGE(TAG, "Got empty connection info");
            }
        } break;
        case WIFI_DISCONNECT:
            wifi_disconnect();
            break;
        case WEBRTC_START: {
            ESP_LOGI(TAG, "WEBRTC_START");
            char *api_key = (char *)ev.data;
            if (api_key) {
                ESP_LOGD(TAG, "API key: %s", api_key);
                strcpy(s_app_ctx.openai_ctx->api_key, api_key);
                if (!(xEventGroupGetBits(app_status_bits) &
                      STATUS_WIFI_CONNECTED_MSK)) {
                    LVGL_MUTEX_LOCKED_CALL(
                        msg_box_create("Not connected to wifi"));
                } else {
                    xMessageBufferReset(s_app_ctx.openai_ctx->messages_buffer);
                    xMessageBufferReset(s_app_ctx.openai_ctx->transcripts_buffer);
                    memset(s_app_ctx.openai_ctx->previous_response_id, 0,
                           sizeof(s_app_ctx.openai_ctx->previous_response_id));
                    LVGL_MUTEX_LOCKED_CALL(
                        processing_popup_show(s_app_ctx.main_cont->processing_popup, 1));
                    int ret = start_webrtc(s_app_ctx.openai_ctx);
                    if (ret != 0) {
                        ESP_LOGE(TAG, "ret=%d", ret);
                        lvgl_port_lock(-1);
                        processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                        msg_box_create("Failed to start webrtc");
                        lvgl_port_unlock();
                    }
                }
                free(api_key);
            } else {
                LVGL_MUTEX_LOCKED_CALL(msg_box_create("Provide API key"));
            }
        } break;
        case WEBRTC_STOP:
            ESP_LOGI(TAG, "WEBRTC_STOP");
            stop_webrtc();
            break;
        case STORAGE_STORE_WIFI_SETTINGS:
            ESP_LOGI(TAG, "STORAGE_STORE_WIFI_SETTINGS");
            nvs_store_str(APP_SETTINGS_PART_NAME, WIFI_SSID_KEY,
                          s_app_ctx.network_info.ssid);
            nvs_store_str(APP_SETTINGS_PART_NAME, WIFI_PSW_KEY,
                          s_app_ctx.network_info.password);
            break;
        case STORAGE_STORE_API_KEY:
            ESP_LOGI(TAG, "STORAGE_STORE_API_KEY");
            nvs_store_str(APP_SETTINGS_PART_NAME, API_KEY_KEY,
                          s_app_ctx.openai_ctx->api_key);
            break;
        case STORAGE_CLEAR:
            ESP_LOGI(TAG, "STORAGE_CLEAR");
            nvs_erase_data(APP_SETTINGS_PART_NAME);
            break;
        default:
            break;
        }
    }
    media_lib_thread_destroy(NULL);
}

static void wifi_status_monitor(void *pv) {
    for (;;) {
        EventBits_t xBits = xEventGroupWaitBits(
            wifi_event_group, WIFI_STATE_UPDATE_BIT | WIFI_SCAN_DONE_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);
        ESP_LOGD(__FUNCTION__, "bits=%lu", xBits);
        if (xBits & WIFI_SCAN_DONE_BIT) {
            char *ssids;
            int n = get_available_networks(&ssids);
            lvgl_port_lock(-1);
            if (n > 0) {
                add_wifi_options(s_app_ctx.main_cont->config_screen->wifi_win, ssids, n);
                free(ssids);
            }
            processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
            lvgl_port_unlock();
        } else {
            uint8_t prev_state =
                xEventGroupGetBits(app_status_bits) & STATUS_WIFI_CONNECTED_MSK;
            uint8_t connected = (xBits & WIFI_STATE_CONNECTED_BIT);
            lvgl_port_lock(-1);
            if (prev_state != connected) {
                processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                status_bar_event_t ev = {.type = SB_WIFI, .value = connected};
                status_bar_update_state(s_app_ctx.main_cont->status_bar, ev);
                wifi_window_update(s_app_ctx.main_cont->config_screen->wifi_win,
                                   connected);
                if (connected) {
                    xEventGroupSetBits(app_status_bits,
                                       STATUS_WIFI_CONNECTED_MSK);
                    AppEvent_t ev = {.id = STORAGE_STORE_WIFI_SETTINGS,
                                     .data = NULL};
                    xQueueSend(app_events_queue, &ev, 0);
                } else {
                    EventBits_t xBits = xEventGroupClearBits(
                        app_status_bits, STATUS_WIFI_CONNECTED_MSK);
                    if (xBits & STATUS_WEBRTC_CONNECTED_MSK) {
                        ESP_LOGW(TAG, "webrtc is started, try to stop");
                        AppEvent_t ev = {.id = WEBRTC_STOP, .data = NULL};
                        xQueueSend(app_events_queue, &ev, 0);
                    }
                }
            } else if (prev_state == 0 && connected == 0) {
                processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                msg_box_create("Unable to connect");
            }
            lvgl_port_unlock();
        }
    }
    media_lib_thread_destroy(NULL);
}

static void webrtc_status_monitor(void *pv) {
    for (;;) {
        esp_webrtc_event_t ev;
        if (xQueueReceive(s_app_ctx.openai_ctx->webrtc_event_queue, &ev,
                          portMAX_DELAY)) {
            ESP_LOGD(__FUNCTION__, "recv ev=%d", ev.type);
            if (ev.body) {
                ESP_LOGD(__FUNCTION__, "body=%s", ev.body);
            }
            uint8_t prev_state = xEventGroupGetBits(app_status_bits) &
                                 STATUS_WEBRTC_CONNECTED_MSK;
            lvgl_port_lock(-1);
            switch (ev.type) {
            case ESP_WEBRTC_EVENT_CONNECTED: {
                processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                status_bar_event_t sb_ev = {.type = SB_WEBRTC, .value = 1};
                status_bar_update_state(s_app_ctx.main_cont->status_bar, sb_ev);
                terminal_output_clear(s_app_ctx.main_cont->terminal);
                switch_screen(s_app_ctx.main_cont, TERMINAL_SCREEN);
                xEventGroupSetBits(app_status_bits,
                                   STATUS_WEBRTC_CONNECTED_MSK);
                AppEvent_t ev = {.id = STORAGE_STORE_API_KEY, .data = NULL};
                xQueueSend(app_events_queue, &ev, 0);
            } break;
            case ESP_WEBRTC_EVENT_DISCONNECTED: {
                if (prev_state == 0) {
                    processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                    msg_box_create("Faild to connect to OpenAI");
                } else {
                    status_bar_event_t ev = {.type = SB_WEBRTC, .value = 0};
                    status_bar_update_state(s_app_ctx.main_cont->status_bar, ev);
                    switch_screen(s_app_ctx.main_cont, CONFIG_SCREEN);
                    xEventGroupClearBits(app_status_bits,
                                         STATUS_WEBRTC_CONNECTED_MSK);
                }
            } break;
            case ESP_WEBRTC_EVENT_CONNECT_FAILED:
                processing_popup_show(s_app_ctx.main_cont->processing_popup, 0);
                msg_box_create("Faild to connect to OpenAI");
                break;
            default:
                break;
            }
            lvgl_port_unlock();
        }
    }
    media_lib_thread_destroy(NULL);
}

static void openai_events_monitor(void *pv) {
    for (;;) {
        openai_event_t ev;
        if (xQueueReceive(s_app_ctx.openai_ctx->openai_event_queue, &ev,
                          portMAX_DELAY)) {
            ESP_LOGD(__FUNCTION__, "recv ev=%d", ev.type);
            switch (ev.type) {
            case OPENAI_EVENT_TRANSCRIPTION_DONE: {
            } break;
            case OPENAI_EVENT_REQUEST_SENT: {
                LVGL_MUTEX_LOCKED_CALL(terminal_update_icon(
                    s_app_ctx.main_cont->terminal, TERM_ICON_PROCESSING));
            } break;
            case OPENAI_EVENT_REQUEST_TIMEOUT: {
                const char *timeout_msg = "API request timed out\n";
                lvgl_port_lock(-1);
                terminal_output_add(s_app_ctx.main_cont->terminal, timeout_msg,
                                    strlen(timeout_msg));
                terminal_update_icon(s_app_ctx.main_cont->terminal, TERM_ICON_READY);
                lvgl_port_unlock();
            } break;
            case OPENAI_EVENT_RESPONSE_DONE: {
                LVGL_MUTEX_LOCKED_CALL(
                    terminal_update_icon(s_app_ctx.main_cont->terminal, TERM_ICON_READY));
            } break;
            case OPENAI_EVENT_NONE:
            default:
                break;
            }
        }
    }
    media_lib_thread_destroy(NULL);
}

static void openai_transcripts_monitor(void *pv) {
    char *buffer = (char *)heap_caps_malloc(64 + 1, MALLOC_CAP_SPIRAM);
    for (;;) {
        size_t bytes =
            xMessageBufferReceive(s_app_ctx.openai_ctx->transcripts_buffer,
                                  buffer, 64, portMAX_DELAY);
        if (bytes) {
            const char *newline_pos = strchr(buffer, '\n');
            if (newline_pos) {
                ESP_LOGD(TAG, "truncate newline");
                bytes--;
            }
            buffer[bytes] = '\0';
            bytes++;
            LVGL_MUTEX_LOCKED_CALL(
                terminal_transcript_add(s_app_ctx.main_cont->terminal, buffer, bytes));
        }
    }
    heap_caps_free(buffer);
    media_lib_thread_destroy(NULL);
}

static void openai_messages_monitor(void *pv) {
    char *buffer = (char *)heap_caps_malloc(512, MALLOC_CAP_SPIRAM);
    for (;;) {
        size_t bytes =
            xMessageBufferReceive(s_app_ctx.openai_ctx->messages_buffer,
                                  buffer, 512, portMAX_DELAY);
        LVGL_MUTEX_LOCKED_CALL(
            terminal_output_add(s_app_ctx.main_cont->terminal, buffer, bytes));
    }
    heap_caps_free(buffer);
    media_lib_thread_destroy(NULL);
}

static void webrtc_query_task(void *pv) {
    for (;;) {
        media_lib_thread_sleep(2000);
        query_webrtc();
    }
}

static void thread_scheduler(const char *thread_name,
                             media_lib_thread_cfg_t *thread_cfg) {
    if (strcmp(thread_name, "pc_task") == 0) {
        thread_cfg->stack_size = 8 * 1024;
        thread_cfg->priority = 2;
    } else if (strcmp(thread_name, "pc_send") == 0) {
        thread_cfg->stack_size = 4 * 1024;
        thread_cfg->priority = 3;
    } else if (strcmp(thread_name, "Adec") == 0) {
        thread_cfg->stack_size = 1 * 1024;
        thread_cfg->priority = 4;
    } else if (strcmp(thread_name, "aenc") == 0) {
        thread_cfg->stack_size = 32 * 1024;
        thread_cfg->priority = 4;
    } else if (strcmp(thread_name, "ARender") == 0) {
        thread_cfg->stack_size = 1 * 1024;
        thread_cfg->priority = 5;
    } else if (strcmp(thread_name, "AUD_SRC") == 0) {
        thread_cfg->stack_size = 3 * 1024;
        thread_cfg->priority = 5;
    } else if (strcmp(thread_name, "afe_feed") == 0) {
        thread_cfg->stack_size = 8 * 1024;
        thread_cfg->priority = 4;
    } else if (strcmp(thread_name, "afe_fetch") == 0) {
        thread_cfg->stack_size = 2 * 1024;
        thread_cfg->priority = 4;
    } else {
        ESP_LOGI(TAG, "Not found cfg for thread %s, use defaults", thread_name);
    }
}

int app_init() {
    media_lib_add_default_adapter();
    media_lib_thread_set_schedule_cb(thread_scheduler);

    memset(&s_app_ctx, 0, sizeof(s_app_ctx));

    lvgl_port_lock(-1);
    s_app_ctx.main_cont = ui_create();
    lvgl_port_unlock();

    init_board();
    media_sys_buildup();
    if (openai_init(&s_app_ctx.openai_ctx) < 0) {
        return -1;
    }

    if (nvs_init() < 0) {
        return -1;
    }

    if (wifi_init() < 0) {
        ESP_LOGE(TAG, "Unable to init wifi");
        return -1;
    }

    app_status_bits = xEventGroupCreate();
    if (!app_status_bits) {
        ESP_LOGE(TAG, "Unable to crate status bits");
        return -1;
    }
    app_events_queue = xQueueCreate(10, sizeof(AppEvent_t));
    if (!app_events_queue) {
        ESP_LOGE(TAG, "Unable to crate app event queue");
        return -1;
    }

    media_lib_thread_create(NULL, "wifi_status_monitor", wifi_status_monitor,
                            NULL, 3 * 1024, 1, threadNO_AFFINITY);
    media_lib_thread_create(NULL, "webrtc_status_monitor",
                            webrtc_status_monitor, NULL, 3 * 1024, 1,
                            threadNO_AFFINITY);
    media_lib_thread_create(NULL, "openai_events_monitor",
                            openai_events_monitor, NULL, 3 * 1024, 1,
                            threadNO_AFFINITY);
    media_lib_thread_create(NULL, "openai_messages_monitor",
                            openai_messages_monitor, NULL, 3 * 1024, 1,
                            threadNO_AFFINITY);
    media_lib_thread_create(NULL, "openai_transcripts_monitor",
                            openai_transcripts_monitor, NULL, 2 * 1024, 1,
                            threadNO_AFFINITY);
    media_lib_thread_create(NULL, "webrtc_query_task", webrtc_query_task, NULL,
                            4 * 1024, 1, threadNO_AFFINITY);
    // Create a FreeRTOS task to ensure that the stack is allocated in internal
    // RAM
    xTaskCreate(app_events_task, "app_events_task", 6 * 1024, NULL, 1, NULL);

    char *ssid, *password, *api_key;
    if (nvs_load_str(APP_SETTINGS_PART_NAME, WIFI_SSID_KEY, &ssid) &&
        nvs_load_str(APP_SETTINGS_PART_NAME, WIFI_PSW_KEY, &password)) {
        lvgl_port_lock(-1);
        add_wifi_options(s_app_ctx.main_cont->config_screen->wifi_win, ssid, 1);
        set_password(s_app_ctx.main_cont->config_screen->wifi_win, password);
        lvgl_port_unlock();

        selected_network_info_t *s =
            (selected_network_info_t *)malloc(sizeof(selected_network_info_t));
        strncpy(s->ssid, ssid, sizeof(s->ssid));
        strncpy(s->password, password, sizeof(s->password));
        AppEvent_t ev = {.id = WIFI_CONNECT, .data = s};
        xQueueSend(app_events_queue, &ev, 0);

        free(ssid);
        free(password);
    } else {
        ESP_LOGI(TAG, "No saved wifi settings");
    }
    if (nvs_load_str(APP_SETTINGS_PART_NAME, API_KEY_KEY, &api_key)) {
        LVGL_MUTEX_LOCKED_CALL(
            set_api_key(s_app_ctx.main_cont->config_screen->auth_win, api_key));
        strcpy(s_app_ctx.openai_ctx->api_key, api_key);
        free(api_key);
    } else {
        ESP_LOGI(TAG, "No saved api key");
    }
    return 0;
}
