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

    config_destroy(&cfg);

load_out_config_err:
    return;
}
