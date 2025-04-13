#include "esp_display_panel.hpp"
#include "esp_lib_utils.h"
#include "esp_log.h"
#include "i2c_bus.h"

#include "app.h"
#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char *TAG = "main";

#define I2C_NUM            I2C_NUM_0
#define I2C_MASTER_SDA_IO  GPIO_NUM_15 /*!< gpio number for I2C master clock */
#define I2C_MASTER_SCL_IO  GPIO_NUM_16 /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 400000      /*!< I2C master clock frequency */

extern "C" void app_main() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master =
            {
                .clk_speed = I2C_MASTER_FREQ_HZ,
            },
    };

    i2c_bus_handle_t i2c_bus = i2c_bus_create(I2C_NUM, &conf);

    Board *board = new Board();
    assert(board);

    ESP_LOGI(TAG, "Initializing board");
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    ESP_LOGI(TAG, "Initializing LVGL");
    ESP_UTILS_CHECK_FALSE_EXIT(
        lvgl_port_init(board->getLCD(), board->getTouch()), "LVGL init failed");

    if (app_init() < 0) {
        ESP_LOGE(TAG, "Unable to init app");
        return;
    }
}
