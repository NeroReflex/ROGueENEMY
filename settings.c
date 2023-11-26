#include "settings.h"

#include <libconfig.h>

void init_config(controller_settings_t *const conf) {

}

int fill_config(controller_settings_t *const conf, const char* file) {
    int res = 0;

    config_t cfg;

    const int config_read_res = config_read_file(&cfg, file);
    if (config_read_res != CONFIG_TRUE) {
        fprintf(stderr, "Error in reading config file: %s\n", config_error_text(&cfg));
        goto fill_config_err;
    }

    int enable_qam;
    if (config_lookup_bool(&cfg, "enable_qam", &enable_qam) != CONFIG_FALSE) {
        conf->enable_qam = enable_qam;
    } else {
        fprintf(stderr, "enable_qam (bool) configuration not found. Default value will be used.\n");
    }

    int ff_gain;
    if (config_lookup_int(&cfg, "ff_gain", &ff_gain) != CONFIG_FALSE) {
        if (ff_gain <= 100) {
            conf->ff_gain = (ff_gain == 100) ? 0xFFFF : ((int)0xFFFF / 100) * ff_gain;
        } else {
            fprintf(stderr, "ff_gain (int) must be a number between 0 and 100");
        }
    } else {
        fprintf(stderr, "ff_gain (int) configuration not found. Default value will be used.\n");
    }

fill_config_err:
    return res;
}