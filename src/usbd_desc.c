#include "usbd_desc.h"
#include "usbd_core.h"
#define VID 0xC251U
#define PID 0x1C03U
#define LANG 0x0409U
static uint8_t dev[USB_LEN_DEV_DESC] = {18,
                                        USB_DESC_TYPE_DEVICE,
                                        0x00,
                                        0x02,
                                        0,
                                        0,
                                        0,
                                        USB_MAX_EP0_SIZE,
                                        LOBYTE(VID),
                                        HIBYTE(VID),
                                        LOBYTE(PID),
                                        HIBYTE(PID),
                                        0x01,
                                        0x01,
                                        1,
                                        2,
                                        3,
                                        1};
static uint8_t lang[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING, LOBYTE(LANG), HIBYTE(LANG)};
static uint8_t str[USBD_MAX_STR_DESC_SIZ];
static uint8_t serial[] = {26,  USB_DESC_TYPE_STRING,
                           'W', 0,
                           'S', 0,
                           '2', 0,
                           '8', 0,
                           '1', 0,
                           '2', 0,
                           '-', 0,
                           '0', 0,
                           '0', 0,
                           '0', 0,
                           '1', 0};
static uint8_t *device(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  *l = sizeof dev;
  return dev;
}
static uint8_t *langs(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  *l = sizeof lang;
  return lang;
}
static uint8_t *mk(const char *x, uint16_t *l) {
  USBD_GetString((uint8_t *)x, str, l);
  return str;
}
static uint8_t *mfg(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  return mk("Keil", l);
}
static uint8_t *product(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  return mk("MCBSTM32 Memory", l);
}
static uint8_t *ser(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  *l = sizeof serial;
  return serial;
}
static uint8_t *cfg(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  return mk("MSC Config", l);
}
static uint8_t *iface(USBD_SpeedTypeDef s, uint16_t *l) {
  (void)s;
  return mk("MSC Interface", l);
}
USBD_DescriptorsTypeDef MSC_Desc = {device, langs, mfg, product, ser, cfg, iface};
