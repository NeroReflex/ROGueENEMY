#pragma once

#include "rogue_enemy.h"

typedef struct controller_settings {
    uint16_t ff_gain;
    int enable_qam;
    int nintendo_layout;

    /**
     * 0 is virtual evdev
     * 1 is DualSense
     * 2 is DualShock
     * 3 is Xbox one
     */
    int gamepad_output_device;
} controller_settings_t;

void init_config(controller_settings_t *const conf);

int fill_config(controller_settings_t *const conf, const char* file);
