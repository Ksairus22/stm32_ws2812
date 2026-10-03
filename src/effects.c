#include "effects.h"
#include "app_config.h"
#include "ws2812.h"

static uint32_t rng, last_mode, mode_start_ms;
static const uint8_t sequence[] = {APP_EFFECT_SEQUENCE};
static const uint32_t durations_ms[] = {APP_EFFECT_DURATIONS_MS};
#define EFFECT_SEQUENCE_LENGTH (sizeof(sequence) / sizeof(sequence[0]))
static const char *names[EFFECT_MODE_COUNT] = {
    "Static", "Hue sweep", "Comet", "Rainbow", "Sparkle", "Breathing",
    "Fire", "Twinkle", "Larson scanner", "Color waves", "Theater chase", "Ocean"};

static uint32_t rnd(void) { rng = rng * 1664525UL + 1013904223UL; return rng; }
const char *effects_name(void) { return names[sequence[last_mode]]; }
void effects_init(uint32_t seed) { rng = seed ? seed : 1; last_mode = 0; mode_start_ms = 0; }

static ws_color_t hsv(uint8_t h, uint8_t s, uint8_t v) {
  uint8_t region = h / 43, f = (h - region * 43) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * f) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - f)) >> 8))) >> 8;
  switch (region) {
  case 0: return (ws_color_t){v, t, p}; case 1: return (ws_color_t){q, v, p};
  case 2: return (ws_color_t){p, v, t}; case 3: return (ws_color_t){p, q, v};
  case 4: return (ws_color_t){t, p, v}; default: return (ws_color_t){v, p, q};
  }
}
static uint8_t wave(uint32_t x) { x &= 255U; return (uint8_t)(x < 128U ? x * 2U : 255U - (x - 128U) * 2U); }

static void draw(uint32_t mode, uint32_t frame, uint8_t fade) {
  for (uint32_t i = 0; i < WS2812_LED_COUNT; ++i) {
    uint8_t pos = (uint8_t)(i * 256U / WS2812_LED_COUNT), v = WS2812_MAX_BRIGHTNESS;
    ws_color_t c = {0, 0, 0};
    switch (mode) {
    case 0: c = (ws_color_t){32, 0, 64}; break;
    case 1: c = hsv((uint8_t)(frame + i * 12U), EFFECT_SATURATION, v); break;
    case 2: { uint8_t d = (uint8_t)(frame + pos); c = hsv(150, EFFECT_SATURATION, d < 128 ? d / 2 : (255 - d) / 2); } break;
    case 3: c = hsv((uint8_t)(frame * 2U + pos), EFFECT_SATURATION, v); break;
    case 4: c = ((i + frame) % 17U == 0U) ? (ws_color_t){v, v, v} : c; break;
    case 5: v = wave(frame); c = (ws_color_t){v / 3, 0, v / 2}; break;
    case 6: { uint8_t heat = (uint8_t)(wave(frame * 3U + pos) * 3U / 4U); c = (ws_color_t){heat, heat / 5, 0}; } break;
    case 7: c = ((rnd() & 31U) == 0U) ? hsv((uint8_t)rnd(), EFFECT_SATURATION, v) : c; break;
    case 8: { uint32_t head = frame % WS2812_LED_COUNT, d = (i + WS2812_LED_COUNT - head) % WS2812_LED_COUNT; c = hsv(160, EFFECT_SATURATION, d < EFFECT_TRAIL_LENGTH ? (uint8_t)(v * (EFFECT_TRAIL_LENGTH - d) / EFFECT_TRAIL_LENGTH) : 0); } break;
    case 9: c = hsv((uint8_t)(frame * 2U + pos * 2U), EFFECT_SATURATION, (uint8_t)(v * (wave(frame + pos) + 40U) / 295U)); break;
    case 10: c = ((i + frame) % 3U == 0U) ? hsv((uint8_t)(frame * 4U), EFFECT_SATURATION, v) : c; break;
    default: c = hsv((uint8_t)(150U + pos / 3U), EFFECT_SATURATION, (uint8_t)(v * (wave(frame + pos / 2U) + 80U) / 335U)); break;
    }
    c.r = (uint8_t)(c.r * fade / 255U); c.g = (uint8_t)(c.g * fade / 255U); c.b = (uint8_t)(c.b * fade / 255U);
    ws2812_set(i, c);
  }
}

void effects_step(uint32_t ms) {
  uint32_t elapsed = ms - mode_start_ms;
  if (elapsed >= durations_ms[last_mode]) { last_mode = (last_mode + 1U) % EFFECT_SEQUENCE_LENGTH; mode_start_ms = ms; elapsed = 0; }
  uint8_t fade = 255;
  if (EFFECT_TRANSITION_MS && elapsed < EFFECT_TRANSITION_MS) fade = (uint8_t)(elapsed * 255U / EFFECT_TRANSITION_MS);
  draw(sequence[last_mode], ms / EFFECT_FRAME_INTERVAL_MS, fade);
  ws2812_show();
}

