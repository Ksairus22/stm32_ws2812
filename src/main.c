#include "effects.h"
#include "stm32f1xx_hal.h"
#include "usb_msc_disk.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage.h"
#include "ws2812.h"
void SystemClock_Config(void);
TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;
PCD_HandleTypeDef hpcd_USB_FS;
static USBD_HandleTypeDef usb;
static void timer_init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  GPIO_InitTypeDef g = {.Pin = GPIO_PIN_8,
                        .Mode = GPIO_MODE_AF_PP,
                        .Pull = GPIO_NOPULL,
                        .Speed = GPIO_SPEED_FREQ_HIGH};
  HAL_GPIO_Init(GPIOA, &g);
  hdma_tim1_ch1.Instance = WS2812_DMA_CHANNEL;
  hdma_tim1_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_tim1_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_tim1_ch1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_tim1_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim1_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_tim1_ch1.Init.Mode = DMA_NORMAL;
  hdma_tim1_ch1.Init.Priority = DMA_PRIORITY_HIGH;
  HAL_DMA_Init(&hdma_tim1_ch1);
  htim1.Instance = WS2812_TIMER;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 89;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&htim1);
  __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC1], hdma_tim1_ch1);
  TIM_OC_InitTypeDef oc = {.OCMode = TIM_OCMODE_PWM1,
                           .Pulse = 0,
                           .OCPolarity = TIM_OCPOLARITY_HIGH,
                           .OCNPolarity = TIM_OCNPOLARITY_HIGH,
                           .OCFastMode = TIM_OCFAST_DISABLE,
                           .OCIdleState = TIM_OCIDLESTATE_RESET,
                           .OCNIdleState = TIM_OCNIDLESTATE_RESET};
  HAL_TIM_PWM_ConfigChannel(&htim1, &oc, WS2812_TIMER_CHANNEL);
}
static void usb_reenumerate(void) {
  GPIO_InitTypeDef g = {.Pin = GPIO_PIN_12,
                        .Mode = GPIO_MODE_OUTPUT_PP,
                        .Pull = GPIO_NOPULL,
                        .Speed = GPIO_SPEED_FREQ_LOW};
  HAL_GPIO_Init(GPIOA, &g);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_Delay(20);
}
int main(void) {
  HAL_Init();
  SystemClock_Config();
  timer_init();
  ws2812_init();
  disk_init();
  effects_init(HAL_GetTick() ^ *(uint32_t *)0x1FFFF7E8U);
  usb_reenumerate();
  /* USB is optional: no return value below can stop the autonomous LED loop. */
  if (USBD_Init(&usb, &MSC_Desc, 0) == USBD_OK) {
    USBD_RegisterClass(&usb, USBD_MSC_CLASS);
    USBD_MSC_RegisterStorage(&usb, &USB_DISK_fops);
    USBD_Start(&usb);
  }
  uint32_t next_frame = 0, next_readme = 0;
  while (1) {
    uint32_t now = HAL_GetTick();
    if ((int32_t)(now - next_frame) >= 0) {
      effects_step(now);
      next_frame = now + 40;
    }
    if ((int32_t)(now - next_readme) >= 0) {
      disk_update_readme(effects_name(), now);
      next_readme = now + 250;
    }
  }
}
