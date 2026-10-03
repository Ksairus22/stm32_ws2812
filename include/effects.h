#pragma once
#include <stdint.h>
#define EFFECT_MODE_COUNT 12U
const char *effects_name(void);
void effects_init(uint32_t seed);
void effects_step(uint32_t elapsed_ms);
