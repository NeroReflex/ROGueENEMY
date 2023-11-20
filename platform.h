#pragma once

#include "rogue_enemy.h"

typedef struct rc71l_platform {
    unsigned long mode;
    unsigned int modes_count;
} rc71l_platform_t;

int init_platform(rc71l_platform_t *const platform);

int cycle_mode(rc71l_platform_t *const platform);

int is_mouse_mode(rc71l_platform_t *const platform);

int is_gamepad_mode(rc71l_platform_t *const platform);

int is_macro_mode(rc71l_platform_t *const platform);

