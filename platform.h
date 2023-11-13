#pragma once

#include "rogue_enemy.h"

typedef struct rc71l_platform {
    unsigned long mode;
    unsigned int modes_count;
} rc71l_platform_t;

#ifdef PLATFORM_FILE
rc71l_platform_t* global_platform = NULL;
#else
extern rc71l_platform_t* global_platform;
#endif

void init_global_mode();

int cycle_mode();

int mouse_mode();

int gamepad_mode();

int macro_mode();