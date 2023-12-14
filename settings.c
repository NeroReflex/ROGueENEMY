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

    config_destroy(&cfg);

load_out_config_err:
    return;
}
