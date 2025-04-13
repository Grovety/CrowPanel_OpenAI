#include "esp_log.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"

#include "audio_codec_data_dummy.h"
#include "esp_codec_dev_defaults.h"

#include "board.h"
#include "dummy_codec.h"

static const char *TAG = "Board";

#define MIC_WS_PIN      GPIO_NUM_2
#define MIC_DATA_IN_PIN GPIO_NUM_20
#define MIC_BCLK_PIN    GPIO_NUM_19

#define SPK_BLK_PIN      GPIO_NUM_40
#define SPK_WS_PIN       GPIO_NUM_39
#define SPK_DATA_OUT_PIN GPIO_NUM_41
#define SPK_DATA_IN_PIN  GPIO_NUM_NC
#define SPK_SCLK_PIN     GPIO_NUM_NC

#define USE_STD_MIC 1

#define I2S_MAX_KEEP SOC_I2S_NUM

typedef struct {
    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;
} i2s_keep_t;

static i2s_keep_t *i2s_keep[I2S_MAX_KEEP];

static esp_codec_dev_handle_t in_dev = NULL;
static esp_codec_dev_handle_t out_dev = NULL;

static int i2s_rx_init(uint8_t port) {
    if (port >= I2S_MAX_KEEP) {
        return -1;
    }
    // Already installed
    if (i2s_keep[port]) {
        return 0;
    }
    i2s_keep[port] = (i2s_keep_t *)calloc(1, sizeof(i2s_keep_t));
    if (i2s_keep[port] == NULL) {
        return -1;
    }
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)port, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 160;
    ESP_ERROR_CHECK(
        i2s_new_channel(&chan_cfg, NULL, &i2s_keep[port]->rx_handle));
#if USE_PDM_MIC
    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                   I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .clk = MIC_WS_PIN,
                .din = MIC_DATA_IN_PIN,
            },
    };
    ESP_ERROR_CHECK(
        i2s_channel_init_pdm_rx_mode(i2s_keep[port]->rx_handle, &pdm_cfg));
#elif USE_STD_MIC
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk = GPIO_NUM_NC,
                .bclk = MIC_BCLK_PIN,
                .ws = MIC_WS_PIN,
                .dout = GPIO_NUM_NC,
                .din = MIC_DATA_IN_PIN,
            },
    };
    ESP_ERROR_CHECK(
        i2s_channel_init_std_mode(i2s_keep[port]->rx_handle, &std_cfg));
#else
#error "unknown mic"
#endif
    return 0;
}

void init_board(void) {
    ESP_LOGI(TAG, "Init board.");

    if (i2s_rx_init(0) < 0) {
        printf("unable to init i2s\n");
        return;
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = i2s_keep[0]->rx_handle,
        .tx_handle = NULL,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    dummy_codec_cfg_t codec_cfg = {};
    const audio_codec_if_t *codec_if = dummy_codec_new(&codec_cfg);
    esp_codec_dev_cfg_t in_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    in_dev = esp_codec_dev_new(&in_dev_cfg);

    // player is not used, so create dummy interface
    audio_codec_data_dummy_cfg_t dummy_data_cfg = {};
    const audio_codec_data_if_t *dummy_data_if =
        audio_codec_new_dummy_data(&dummy_data_cfg);
    esp_codec_dev_cfg_t out_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = dummy_data_if,
    };
    out_dev = esp_codec_dev_new(&out_dev_cfg);
}

esp_codec_dev_handle_t get_playback_handle(void) { return out_dev; }

esp_codec_dev_handle_t get_record_handle(void) { return in_dev; }
