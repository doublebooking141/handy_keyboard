#include "hal_audio.h"
#include "board_config.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "hal_audio";
static i2s_chan_handle_t tx_handle = NULL;

#define ES8311_ADDR 0x18  // 7-bit address (0x30 >> 1)

static esp_err_t es8311_write_reg(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 write failed: reg=0x%02x, err=%d", reg, ret);
    }
    return ret;
}

static void es8311_init_sequence() {
    ESP_LOGI(TAG, "Configuring ES8311...");
    
    // 1. Reset
    es8311_write_reg(0x00, 0x1F); // Reset all
    vTaskDelay(20 / portTICK_PERIOD_MS);
    es8311_write_reg(0x00, 0x00); // Release reset

    // 2. Clock Management
    es8311_write_reg(0x01, 0x3F); // MCLK_ON, BCLK_ON, CLK_ADC_ON, CLK_DAC_ON, MCLK_DIV=1
    es8311_write_reg(0x02, 0x00); // DIG_CLK_DIV=0
    es8311_write_reg(0x03, 0x10); // ADC_CLK_DIV=0
    es8311_write_reg(0x04, 0x10); // DAC_CLK_DIV=0
    es8311_write_reg(0x05, 0x00); // Master Mode off (Slave), BCLK/LRCK Div=0

    // 3. Power Management
    es8311_write_reg(0x0D, 0x01); // Power Up Analog (PGA, ADC, DAC, etc)
    es8311_write_reg(0x0E, 0xBF); // Power Up Digital (ADC, DAC, etc) - Enable all

    // 4. Format Setting (I2S, 16bit)
    // Reg 0x09: ADC Control 1. Bit4:3=00 (I2S), Bit2:1=11 (16bit) -> 0x0C
    es8311_write_reg(0x09, 0x0C); 
    // Reg 0x0A: DAC Control 1. Bit4:3=00 (I2S), Bit2:1=11 (16bit) -> 0x0C
    es8311_write_reg(0x0A, 0x0C);

    // 5. Volume Control (Default 80%)
    // 0x00: -95.5dB, 0xBF: 0dB. 0x99 is approx 80% of the range.
    es8311_write_reg(0x14, 0x00); // DAC Digital Gain 0dB
    es8311_write_reg(0x32, 0x99); // DAC Volume (Approx 80%)

    // 6. State Machine Start
    es8311_write_reg(0x00, 0x80); // CSM On
    
    ESP_LOGI(TAG, "ES8311 Configured (Default Volume 80%%)");
}

esp_err_t hal_audio_init(void) {
    // I2C for Codec
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_I2C_SDA,
        .scl_io_num = BOARD_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // PA Pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_PA_EN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(BOARD_PA_EN, 1); // Enable PA

    // Init Codec Chip
    es8311_init_sequence();

    // I2S
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = BOARD_I2S_MCLK, .bclk = BOARD_I2S_BCK, .ws = BOARD_I2S_WS, .dout = BOARD_I2S_DOUT, .din = BOARD_I2S_DIN,
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
    return ESP_OK;
}

esp_err_t hal_audio_write_i2s(const void* data, size_t size, size_t* bytes_written, uint32_t timeout_ms) {
    return i2s_channel_write(tx_handle, data, size, bytes_written, timeout_ms);
}
