#include "legion_go.h"
#include "input_dev.h"
#include "dev_hidraw.h"
#include "xbox360.h"

static input_dev_t in_xbox_dev = {
    .dev_type = input_dev_type_uinput,
    .filters = {
        .ev = {
          .name = "Generic X-Box pad"
        }
    },
    .ev_input_map_fn = xbox360_ev_map,
};

static xbox360_settings_t x360_cfg;

static input_dev_t in_iio_dev = {
    .dev_type = input_dev_type_iio,
    .filters = {
        .iio = {
            .name = "gyro_3d",
        }
    },
};

static int legion_platform_init(void** platform_data) {
    int res = -EINVAL;
    return 0;
}
static void legion_platform_deinit(void** platform_data) {

    // free(platform);
    *platform_data = NULL;
}
input_dev_composite_t legion_composite = {
    .dev = {
        &in_xbox_dev,
        // &in_iio_dev,
    },
    .dev_count = 1,
    .init_fn = legion_platform_init,
    .deinit_fn = legion_platform_deinit,
};


input_dev_composite_t* legion_go_device_def(const controller_settings_t *const settings) {
    // x360_cfg.nintendo_layout = settings->nintendo_layout;

    in_xbox_dev.user_data = (void*)&x360_cfg;
    return &legion_composite;
}



// add properties on on devices_status.h
// and reuse the message with buttons
// then do filtering and wizardry xScale into the message read
// so that report generation is the fastest possible
