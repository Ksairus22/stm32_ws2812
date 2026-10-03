#pragma once
#include "app_config.h"
#include "stm32f1xx_hal.h"
typedef struct {
  uint8_t r, g, b;
} ws_color_t;
void ws2812_init(void);
void ws2812_set(uint32_t index, ws_color_t color);
void ws2812_clear(void);
void ws2812_show(void);
