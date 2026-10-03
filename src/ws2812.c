#include "ws2812.h"
#include <string.h>
extern TIM_HandleTypeDef htim1;
static ws_color_t pixels[WS2812_LED_COUNT];
static uint16_t dma_buf[WS2812_LED_COUNT * 24U + 64U];
void ws2812_init(void) {
  memset(pixels, 0, sizeof pixels);
}
void ws2812_set(uint32_t i, ws_color_t c) {
  if (i < WS2812_LED_COUNT)
    pixels[i] = c;
}
void ws2812_clear(void) {
  memset(pixels, 0, sizeof pixels);
}
void ws2812_show(void) {
  uint32_t n = 0;
  for (uint32_t i = 0; i < WS2812_LED_COUNT; i++) {
    uint8_t v[3] = {pixels[i].g, pixels[i].r, pixels[i].b};
    for (uint32_t k = 0; k < 3; k++)
      for (int b = 7; b >= 0; b--)
        dma_buf[n++] = (v[k] & (1U << b)) ? 56U : 28U;
  }
  while (n < sizeof(dma_buf) / sizeof(dma_buf[0]))
    dma_buf[n++] = 0;
  HAL_TIM_PWM_Start_DMA(&htim1, WS2812_TIMER_CHANNEL, (uint32_t *)dma_buf, n);
  HAL_Delay(2);
  HAL_TIM_PWM_Stop_DMA(&htim1, WS2812_TIMER_CHANNEL);
}
