#include "usbd_storage.h"
#include "app_config.h"
#include "usb_msc_disk.h"
#include <stdio.h>
#include <string.h>
#define FAT_SECTORS 2U
#define ROOT_SECTOR (1U + 2U * FAT_SECTORS)
#define DATA_SECTOR (ROOT_SECTOR + 1U)
#define LOG_LINE_SIZE 32U
#define LOG_CAPACITY 8000U
#define LOG_PACKED_SIZE ((LOG_CAPACITY * 3U + 7U) / 8U)
static uint8_t boot[512], fat[FAT_SECTORS * 512U], root[512];
static uint8_t mode_history[LOG_PACKED_SIZE];
static uint16_t history_count;
static uint8_t last_logged_mode = 0xFFU;
static const char log_header[] = "STM32F103C8T6 WS2812 controller\r\n\r\nTime      Mode\r\n";
static const char *const mode_names[] = {
    "Static", "Hue sweep", "Comet", "Rainbow", "Sparkle", "Breathing"};
static int8_t inquiry[36] = {0x00, 0x80, 0x02, 0x02, 31,  0,   0,   0,   'K', 'e', 'i', 'l',
                             ' ',  ' ',  ' ',  ' ',  'M', 'C', 'B', 'S', 'T', 'M', '3', '2',
                             ' ',  'D',  'i',  's',  'k', ' ', ' ', ' ', '1', '.', '0', ' '};
static void le16(uint8_t *p, uint16_t v) {
  p[0] = v;
  p[1] = v >> 8;
}
static void le32(uint8_t *p, uint32_t v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
}
static void fat12_set(uint16_t cluster, uint16_t value) {
  uint32_t p = cluster + (cluster / 2U);
  if (cluster & 1U) {
    fat[p] = (fat[p] & 0x0FU) | (uint8_t)(value << 4);
    fat[p + 1] = (uint8_t)(value >> 4);
  } else {
    fat[p] = (uint8_t)value;
    fat[p + 1] = (fat[p + 1] & 0xF0U) | (uint8_t)(value >> 8);
  }
}
static void history_set(uint16_t index, uint8_t mode) {
  uint32_t bit = (uint32_t)index * 3U, p = bit / 8U, shift = bit & 7U;
  uint16_t v = (uint16_t)mode << shift;
  uint16_t mask = (uint16_t)7U << shift;
  uint16_t pair = mode_history[p];
  if (p + 1U < LOG_PACKED_SIZE)
    pair |= (uint16_t)mode_history[p + 1U] << 8;
  pair = (pair & ~mask) | v;
  mode_history[p] = (uint8_t)pair;
  if (p + 1U < LOG_PACKED_SIZE)
    mode_history[p + 1U] = (uint8_t)(pair >> 8);
}
static uint8_t history_get(uint16_t index) {
  uint32_t bit = (uint32_t)index * 3U, p = bit / 8U, shift = bit & 7U;
  uint16_t pair = mode_history[p];
  if (p + 1U < LOG_PACKED_SIZE)
    pair |= (uint16_t)mode_history[p + 1U] << 8;
  return (uint8_t)((pair >> shift) & 7U);
}
static uint32_t log_size(void) {
  return (uint32_t)(sizeof(log_header) - 1U) + (uint32_t)history_count * LOG_LINE_SIZE;
}
static void update_filesystem(void) {
  uint32_t bytes = log_size(), clusters = (bytes + 511U) / 512U;
  memset(fat, 0, sizeof fat);
  fat[0] = 0xF8;
  fat[1] = 0xFF;
  fat[2] = 0xFF;
  for (uint32_t i = 0; i < clusters; i++)
    fat12_set((uint16_t)(i + 2U), (i + 1U < clusters) ? (uint16_t)(i + 3U) : 0xFFFU);
  le32(root + 32 + 28, bytes);
}
static void render_log_sector(uint8_t *out, uint32_t sector) {
  memset(out, 0, 512);
  uint32_t base = sector * 512U, end = base + 512U, header_len = sizeof(log_header) - 1U;
  if (base < header_len) {
    uint32_t n = header_len - base;
    if (n > 512U)
      n = 512U;
    memcpy(out, log_header + base, n);
  }
  uint32_t pos = base > header_len ? base : header_len;
  while (pos < end) {
    uint32_t rel = pos - header_len, line_index = rel / LOG_LINE_SIZE,
             line_pos = rel % LOG_LINE_SIZE;
    if (line_index >= history_count)
      break;
    char line[LOG_LINE_SIZE + 1U];
    uint32_t sec = line_index * EFFECT_FRAME_INTERVAL_MS / 1000U;
    int n = snprintf(line,
                     sizeof line,
                     "%02lu:%02lu:%02lu  %-20s\r\n",
                     (unsigned long)(sec / 3600U),
                     (unsigned long)((sec / 60U) % 60U),
                     (unsigned long)(sec % 60U),
                     mode_names[history_get((uint16_t)line_index)]);
    if (n <= 0)
      break;
    uint32_t chunk = LOG_LINE_SIZE - line_pos;
    if (chunk > end - pos)
      chunk = end - pos;
    memcpy(out + (pos - base), line + line_pos, chunk);
    pos += chunk;
  }
}
void disk_init(void) {
  memset(boot, 0, 512);
  memset(root, 0, 512);
  memset(mode_history, 0, sizeof mode_history);
  history_count = 0;
  last_logged_mode = 0xFFU;
  uint8_t *b = boot;
  b[0] = 0xEB;
  b[1] = 0x3C;
  b[2] = 0x90;
  memcpy(b + 3, "MSDOS5.0", 8);
  le16(b + 11, 512);
  b[13] = 1;
  le16(b + 14, 1);
  b[16] = 2;
  le16(b + 17, 16);
  le16(b + 19, USB_DISK_BLOCKS);
  b[21] = 0xF8;
  le16(b + 22, FAT_SECTORS);
  le16(b + 24, 1);
  le16(b + 26, 1);
  b[38] = 0x29;
  le32(b + 39, 0x27021974);
  memcpy(b + 43, "STM32x_USB ", 11);
  memcpy(b + 54, "FAT12   ", 8);
  b[510] = 0x55;
  b[511] = 0xAA;
  memcpy(root, "STM32x_USB ", 11);
  root[11] = 0x08;
  uint8_t *r = root + 32;
  memcpy(r, "README  TXT", 11);
  r[11] = 0x20;
  le16(r + 26, 2);
  update_filesystem();
}
void disk_update_readme(const char *mode, uint32_t ms) {
  (void)ms;
  uint8_t id = 0xFFU;
  for (uint8_t i = 0; i < 6U; i++)
    if (strcmp(mode, mode_names[i]) == 0) {
      id = i;
      break;
    }
  if (id == 0xFFU || id == last_logged_mode || history_count >= LOG_CAPACITY)
    return;
  __disable_irq();
  history_set(history_count, id);
  history_count++;
  last_logged_mode = id;
  update_filesystem();
  __enable_irq();
}
static int8_t Init(uint8_t l) {
  (void)l;
  return 0;
}
static int8_t Cap(uint8_t l, uint32_t *n, uint16_t *s) {
  (void)l;
  *n = USB_DISK_BLOCKS;
  *s = USB_DISK_BLOCK_SIZE;
  return 0;
}
static int8_t Ready(uint8_t l) {
  (void)l;
  return 0;
}
static int8_t WP(uint8_t l) {
  (void)l;
  return 0;
}
static int8_t Read(uint8_t l, uint8_t *b, uint32_t a, uint16_t n) {
  (void)l;
  if (a + n > USB_DISK_BLOCKS)
    return -1;
  while (n--) {
    if (a == 0)
      memcpy(b, boot, 512);
    else if (a >= 1U && a < 1U + FAT_SECTORS)
      memcpy(b, fat + (a - 1U) * 512U, 512);
    else if (a >= 1U + FAT_SECTORS && a < 1U + 2U * FAT_SECTORS)
      memcpy(b, fat + (a - (1U + FAT_SECTORS)) * 512U, 512);
    else if (a == ROOT_SECTOR)
      memcpy(b, root, 512);
    else if (a >= DATA_SECTOR)
      render_log_sector(b, a - DATA_SECTOR);
    else
      memset(b, 0, 512);
    b += 512;
    a++;
  }
  return 0;
}
static int8_t Write(uint8_t l, uint8_t *b, uint32_t a, uint16_t n) {
  (void)l;
  (void)b;
  if (a + n > USB_DISK_BLOCKS)
    return -1;
  return 0;
}
static int8_t Lun(void) {
  return 0;
}
USBD_StorageTypeDef USB_DISK_fops = {Init, Cap, Ready, WP, Read, Write, Lun, inquiry};
