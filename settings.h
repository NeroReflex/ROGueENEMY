#pragma once

#include "rogue_enemy.h"

typedef struct dev_in_settings {
    bool enable_qam;
    bool rumble_on_mode_switch;
    uint16_t ff_gain;
} dev_in_settings_t;

void load_in_config(dev_in_settings_t *const out_conf, const char* const filepath);

typedef struct dev_out_settings {
    bool nintendo_layout;
    uint8_t default_gamepad;
} dev_out_settings_t;

void load_out_config(dev_out_settings_t *const out_conf, const char* const filepath);