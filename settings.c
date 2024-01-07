#include "settings.h"

#include <libconfig.h>

void load_in_config(dev_in_settings_t *const out_conf, const char* const filepath) {
    config_t cfg;
    config_init(&cfg);

    const int config_read_res = config_read_file(&cfg, filepath);
    if (config_read_res != CONFIG_TRUE) {
        fprintf(stderr, "Error in reading config file: %s\n", config_error_text(&cfg));
        goto load_in_config_err;
    }

    int enable_qam;
    if (config_lookup_bool(&cfg, "enable_qam", &enable_qam) != CONFIG_FALSE) {
        out_conf->enable_qam = enable_qam;
    } else {
        fprintf(stderr, "enable_qam (bool) configuration not found. Default value will be used.\n");
    }

    int rumble_on_mode_switch;
    if (config_lookup_bool(&cfg, "rumble_on_mode_switch", &rumble_on_mode_switch) != CONFIG_FALSE) {
        out_conf->rumble_on_mode_switch = rumble_on_mode_switch;
    } else {
        fprintf(stderr, "rumble_on_mode_switch (bool) configuration not found. Default value will be used.\n");
    }

    int ff_gain;
    if (config_lookup_int(&cfg, "ff_gain", &ff_gain) != CONFIG_FALSE) {
        if (ff_gain <= 0xFF) {
            out_conf->ff_gain = (ff_gain == 0) ? 0x0000 : ((uint16_t)ff_gain << (uint16_t)8) | (uint16_t)0x00FF;
        } else {
            fprintf(stderr, "ff_gain (int) must be a number between 0 and 255");
        }
    } else {
        fprintf(stderr, "ff_gain (int) configuration not found. Default value will be used.\n");
    }

    int m1m2_mode;
    if (config_lookup_int(&cfg, "m1m2_mode", &m1m2_mode) != CONFIG_FALSE) {
        if (m1m2_mode <= 2) {
            out_conf->m1m2_mode = m1m2_mode;
        } else {
            fprintf(stderr, "m1m2_mode (int) must be a number between 0 and 2");
        }
    } else {
        fprintf(stderr, "m1m2_mode (int) configuration not found. Default value will be used.\n");
    }

    int touchbar;
    if (config_lookup_bool(&cfg, "touchbar", &touchbar) != CONFIG_FALSE) {
        out_conf->touchbar = touchbar;
    } else {
        fprintf(stderr, "touchbar (bool) configuration not found. Default value will be used.\n");
    }

    int enable_thermal_profiles_switching;
    if (config_lookup_bool(&cfg, "enable_thermal_profiles_switching", &enable_thermal_profiles_switching) != CONFIG_FALSE) {
        out_conf->enable_thermal_profiles_switching = enable_thermal_profiles_switching;
    } else {
        fprintf(stderr, "enable_thermal_profiles_switching (bool) configuration not found. Default value will be used.\n");
    }

    int default_thermal_profile;
    if (config_lookup_int(&cfg, "default_thermal_profile", &default_thermal_profile) != CONFIG_FALSE) {
        out_conf->default_thermal_profile = default_thermal_profile;
    } else {
        fprintf(stderr, "default_thermal_profile (int) configuration not found. Default value will be used.\n");
    }

    int enable_leds_commands;
    if (config_lookup_bool(&cfg, "enable_leds_commands", &enable_leds_commands) != CONFIG_FALSE) {
        out_conf->enable_leds_commands = enable_leds_commands;
    } else {
        fprintf(stderr, "enable_leds_commands (bool) configuration not found. Default value will be used.\n");
    }

    config_destroy(&cfg);

load_in_config_err:
    return;
}

void load_out_config(dev_out_settings_t *const out_conf, const char* const filepath) {
    config_t cfg;
    config_init(&cfg);

    const int config_read_res = config_read_file(&cfg, filepath);
    if (config_read_res != CONFIG_TRUE) {
        fprintf(stderr, "Error in reading config file: %s\n", config_error_text(&cfg));
        goto load_out_config_err;
    }

    int nintendo_layout;
    if (config_lookup_bool(&cfg, "nintendo_layout", &nintendo_layout) != CONFIG_FALSE) {
        out_conf->nintendo_layout = nintendo_layout;
    } else {
        fprintf(stderr, "nintendo_layout (bool) configuration not found. Default value will be used.\n");
    }

    int default_gamepad;
    if (config_lookup_int(&cfg, "default_gamepad", &default_gamepad) != CONFIG_FALSE) {
        out_conf->default_gamepad = default_gamepad % 3;
    } else {
        fprintf(stderr, "default_gamepad (int) configuration not found. Default value will be used.\n");
    }

    int gamepad_leds_control;
    if (config_lookup_bool(&cfg, "gamepad_leds_control", &gamepad_leds_control) != CONFIG_FALSE) {
        out_conf->gamepad_leds_control = gamepad_leds_control;
    } else {
        fprintf(stderr, "gamepad_leds_control (bool) configuration not found. Default value will be used.\n");
    }

    int gamepad_rumble_control;
    if (config_lookup_bool(&cfg, "gamepad_rumble_control", &gamepad_rumble_control) != CONFIG_FALSE) {
        out_conf->gamepad_rumble_control = gamepad_rumble_control;
    } else {
        fprintf(stderr, "gamepad_rumble_control (bool) configuration not found. Default value will be used.\n");
    }

    int controller_bluetooth;
    if (config_lookup_bool(&cfg, "controller_bluetooth", &controller_bluetooth) != CONFIG_FALSE) {
        out_conf->controller_bluetooth = controller_bluetooth;
    } else {
        fprintf(stderr, "controller_bluetooth (bool) configuration not found. Default value will be used.\n");
    }

    int dualsense_edge;
    if (config_lookup_bool(&cfg, "dualsense_edge", &dualsense_edge) != CONFIG_FALSE) {
        out_conf->dualsense_edge = dualsense_edge;
    } else {
        fprintf(stderr, "dualsense_edge (bool) configuration not found. Default value will be used.\n");
    }

    int swap_y_z;
    if (config_lookup_bool(&cfg, "swap_y_z", &swap_y_z) != CONFIG_FALSE) {
        out_conf->swap_y_z = swap_y_z;
    } else {
        fprintf(stderr, "swap_y_z (bool) configuration not found. Default value will be used.\n");
    }

    int gyro_to_analog_activation_treshold;
    if (config_lookup_int(&cfg, "gyro_to_analog_activation_treshold", &gyro_to_analog_activation_treshold) != CONFIG_FALSE) {
        out_conf->gyro_to_analog_activation_treshold = gyro_to_analog_activation_treshold;
    } else {
        fprintf(stderr, "gyro_to_analog_activation_treshold (int) configuration not found. Default value will be used.\n");
    }

    int gyro_to_analog_mapping;
    if (config_lookup_int(&cfg, "gyro_to_analog_mapping", &gyro_to_analog_mapping) != CONFIG_FALSE) {
        out_conf->gyro_to_analog_mapping = gyro_to_analog_mapping == 0 ? 1 : gyro_to_analog_mapping;
    } else {
        fprintf(stderr, "gyro_to_analog_mapping (int) configuration not found. Default value will be used.\n");
    }

    config_destroy(&cfg);

load_out_config_err:
    return;
}
