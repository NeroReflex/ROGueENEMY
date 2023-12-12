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
    .map = {
        .ev_input_map_fn = xbox360_ev_map,
    }
};

static xbox360_settings_t x360_cfg;

static input_dev_t in_iio_dev = {
    .dev_type = input_dev_type_iio,
    .filters = {
        .iio = {
            .name = "gyro_3d",
        }
    },
    .map = {
        .iio_settings = {
            .sampling_freq_hz = "70.000",
            .post_matrix = {
                {1, 0, 0},
                {0, 1, 0},
                {0, 0, 1}
            }
        }
    }
};

static struct llg_hidraw_data {
    uint8_t last_packet[64];
} llg_hidraw_user_data;

static int llg_hidraw_map(int hidraw_fd, in_message_t *const messages, size_t messages_len, void* user_data) {
    struct llg_hidraw_data *const llg_data = (struct llg_hidraw_data*)user_data;

    int msg_count = 0;

    int read_res = read(hidraw_fd, llg_data->last_packet, sizeof(llg_data->last_packet));
    if (read_res != 64) {
        fprintf(stderr, "Error reading from hidraw device\n");
        return -EINVAL;
    }

    // here we have llg_data->last_packet filled with 64 bytes from the input device
    /*
    const in_message_t current_message = {
      .type = GAMEPAD_SET_ELEMENT,
      .data = {
        .gamepad_set = {
          .element = GAMEPAD_BTN_L5,
          .status = {
            .btn = 1,
          }
        }
      }
    };

    // this does send messages to the output device
    messages[msg_count++] = current_message;
    */

    // successful return
    return msg_count;
}

static input_dev_t in_hidraw_dev = {
    .dev_type = input_dev_type_hidraw,
    .filters = {
        .hidraw = {
            .pid = 0x6182,
            .vid = 0x17ef,
            .rdesc_size = 44,
        }
    },
    .user_data = (void*)&llg_hidraw_user_data,
    .map = {
        .hidraw_input_map_fn = llg_hidraw_map,
    },
};

typedef struct legion_go_platform {
    int _pad;
} legion_go_platform_t;

static int legion_platform_init(void** platform_data) {
    int res = -EINVAL;

    legion_go_platform_t *const llg_platform = malloc(sizeof(legion_go_platform_t));
    if (llg_platform == NULL) {
        fprintf(stderr, "Unable to allocate memory for the platform data\n");
        res = -ENOMEM;
        goto legion_platform_init_err;
    }

    *platform_data = (void*)llg_platform;
    res = 0;

legion_platform_init_err:
    return res;
}

static void legion_platform_deinit(void** platform_data) {
    free(*platform_data);
    *platform_data = NULL;
}

int legion_platform_leds(uint8_t r, uint8_t g, uint8_t b, void* platform_data) {
    return 0;
}

input_dev_composite_t legion_composite = {
    .dev = {
        &in_hidraw_dev,
        &in_xbox_dev,
        &in_iio_dev,
    },
    .dev_count = 3,
    .init_fn = legion_platform_init,
    .leds_fn = legion_platform_leds,
    .deinit_fn = legion_platform_deinit,
};


input_dev_composite_t* legion_go_device_def(const controller_settings_t *const settings) {
    x360_cfg.nintendo_layout = settings->nintendo_layout;

    in_xbox_dev.user_data = (void*)&x360_cfg;
    return &legion_composite;
}



// add properties on on devices_status.h
// and reuse the message with buttons
// then do filtering and wizardry xScale into the message read
// so that report generation is the fastest possible
