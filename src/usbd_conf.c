#include "stm32f1xx_hal.h"
#include "usbd_core.h"
#include "usbd_msc.h"
extern PCD_HandleTypeDef hpcd_USB_FS;
void HAL_PCD_MspInit(PCD_HandleTypeDef *h) {
  (void)h;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USB_CLK_ENABLE();
  GPIO_InitTypeDef g = {.Pin = GPIO_PIN_11 | GPIO_PIN_12,
                        .Mode = GPIO_MODE_AF_INPUT,
                        .Pull = GPIO_NOPULL,
                        .Speed = GPIO_SPEED_FREQ_HIGH};
  HAL_GPIO_Init(GPIOA, &g);
  HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *h) {
  (void)h;
  __HAL_RCC_USB_CLK_DISABLE();
}
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *h) {
  USBD_LL_SetupStage(h->pData, (uint8_t *)h->Setup);
}
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *h, uint8_t ep) {
  USBD_LL_DataOutStage(h->pData, ep, h->OUT_ep[ep].xfer_buff);
}
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *h, uint8_t ep) {
  USBD_LL_DataInStage(h->pData, ep, h->IN_ep[ep].xfer_buff);
}
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *h) {
  USBD_LL_SOF(h->pData);
}
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *h) {
  USBD_LL_SetSpeed(h->pData, USBD_SPEED_FULL);
  USBD_LL_Reset(h->pData);
}
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *h) {
  USBD_LL_Suspend(h->pData);
}
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *h) {
  USBD_LL_Resume(h->pData);
}
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *h, uint8_t ep) {
  USBD_LL_IsoOUTIncomplete(h->pData, ep);
}
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *h, uint8_t ep) {
  USBD_LL_IsoINIncomplete(h->pData, ep);
}
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *h) {
  USBD_LL_DevConnected(h->pData);
}
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *h) {
  USBD_LL_DevDisconnected(h->pData);
}
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *d) {
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = 0;
  hpcd_USB_FS.pData = d;
  d->pData = &hpcd_USB_FS;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
    return USBD_FAIL;
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x00, PCD_SNG_BUF, 0x18);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x80, PCD_SNG_BUF, 0x58);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, MSC_EPIN_ADDR, PCD_SNG_BUF, 0x98);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, MSC_EPOUT_ADDR, PCD_SNG_BUF, 0xD8);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *d) {
  HAL_PCD_DeInit(d->pData);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *d) {
  HAL_PCD_Start(d->pData);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *d) {
  HAL_PCD_Stop(d->pData);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *d, uint8_t a, uint8_t t, uint16_t m) {
  HAL_PCD_EP_Open(d->pData, a, m, t);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *d, uint8_t a) {
  HAL_PCD_EP_Close(d->pData, a);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *d, uint8_t a) {
  HAL_PCD_EP_Flush(d->pData, a);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *d, uint8_t a) {
  HAL_PCD_EP_SetStall(d->pData, a);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *d, uint8_t a) {
  HAL_PCD_EP_ClrStall(d->pData, a);
  return USBD_OK;
}
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *d, uint8_t a) {
  PCD_HandleTypeDef *h = d->pData;
  return (a & 0x80) ? h->IN_ep[a & 0x7F].is_stall : h->OUT_ep[a & 0x7F].is_stall;
}
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *d, uint8_t a) {
  HAL_PCD_SetAddress(d->pData, a);
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *d, uint8_t a, uint8_t *b, uint16_t s) {
  HAL_PCD_EP_Transmit(d->pData, a, b, s);
  return USBD_OK;
}
USBD_StatusTypeDef
USBD_LL_PrepareReceive(USBD_HandleTypeDef *d, uint8_t a, uint8_t *b, uint16_t s) {
  HAL_PCD_EP_Receive(d->pData, a, b, s);
  return USBD_OK;
}
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *d, uint8_t a) {
  return HAL_PCD_EP_GetRxCount(d->pData, a);
}
void USBD_LL_Delay(uint32_t ms) {
  HAL_Delay(ms);
}
void *USBD_static_malloc(uint32_t size) {
  static uint32_t mem[(sizeof(USBD_MSC_BOT_HandleTypeDef) + 3) / 4];
  (void)size;
  return mem;
}
void USBD_static_free(void *p) {
  (void)p;
}
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *h, uint8_t s) {
  (void)h;
  (void)s;
}
