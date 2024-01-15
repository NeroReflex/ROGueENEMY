#pragma once

#include "rogue_enemy.h"

typedef struct dev_in_settings {
    bool enable_qam;
    bool rumble_on_mode_switch;
    uint16_t ff_gain;
    uint8_t m1m2_mode;
    bool touchbar;
    bool enable_thermal_profiles_switching;
    int default_thermal_profile;
    bool enable_leds_commands;
    bool enable_imu;
    bool imu_polling_interface;
} dev_in_settings_t;

void load_in_config(dev_in_settings_t *const out_conf, const char* const filepath);

typedef struct dev_out_settings {
    bool nintendo_layout;
    uint8_t default_gamepad;
    bool gamepad_leds_control;
    bool gamepad_rumble_control;
    bool controller_bluetooth;
    bool dualsense_edge;
    bool swap_y_z;
    bool invert_x;
    int gyro_to_analog_activation_treshold;
    int gyro_to_analog_mapping;
} dev_out_settings_t;

void load_out_config(dev_out_settings_t *const out_conf, const char* const filepath);