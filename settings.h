#pragma once

#include "rogue_enemy.h"

typedef struct controller_settings {
    uint16_t ff_gain;
    int enable_qam;
    int nintendo_layout;
} controller_settings_t;

void init_config(controller_settings_t *const conf);

int fill_config(controller_settings_t *const conf, const char* file);