#pragma once

#include <driver/gpio.h>

// Audio I2S Configurations
#define BOARD_I2S_MCLK      GPIO_NUM_13
#define BOARD_I2S_WS        GPIO_NUM_10
#define BOARD_I2S_BCK       GPIO_NUM_12
#define BOARD_I2S_DIN       GPIO_NUM_48  // P4 RX -> Codec SDOUT
#define BOARD_I2S_DOUT      GPIO_NUM_9   // P4 TX -> Codec SDIN

// Audio Codec (ES8311) I2C Configurations
#define BOARD_I2C_SDA       GPIO_NUM_7
#define BOARD_I2C_SCL       GPIO_NUM_8
#define BOARD_PA_EN         GPIO_NUM_11  // Power Amplifier Enable

// Audio Sample Rate
#define AUDIO_SAMPLE_RATE   16000
