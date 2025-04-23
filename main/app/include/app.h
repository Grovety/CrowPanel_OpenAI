/* App header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _APP_H_
#define _APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#define STATUS_WIFI_CONNECTED_MSK   BIT0
#define STATUS_WEBRTC_CONNECTED_MSK BIT1
#define STATUS_OPENAI_BUSY_MSK      BIT2
#define STATUS_MIC_ON_MSK           BIT3

typedef enum {
    WIFI_START_SCAN = 0,
    WIFI_CONNECT,
    WIFI_DISCONNECT,
    CHAT_SESSION_START,
    CHAT_SESSION_STOP,
    STORAGE_STORE_WIFI_SETTINGS,
    STORAGE_STORE_API_KEY,
    STORAGE_CLEAR,
    MIC_ON,
    MIC_OFF,
} eAppEventId;

typedef struct {
    eAppEventId id;
    union {
        void *data;
        int value;
    };
} AppEvent_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Global app status bits
 */
extern EventGroupHandle_t app_status_bits;

/**
 * @brief  Global app events queue
 */
extern QueueHandle_t app_events_queue;

/**
 * @brief  Init app
 *
 * @return
 *      - -1  On failure
 *      - 0   On success
 */
int app_init();

#ifdef __cplusplus
}
#endif

#endif
