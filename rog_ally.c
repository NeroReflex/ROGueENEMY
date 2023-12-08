#include "rog_ally.h"

void asus_kbd_ev_map(const evdev_collected_t *const e, int in_messages_pipe_fd, void* user_data) {
  in_message_t current_message;

  
  /*
  const ssize_t in_message_pipe_write_res = write(in_messages_pipe_fd, (void*)&current_message, sizeof(in_message_t));
  if (in_message_pipe_write_res != sizeof(in_message_t)) {
      fprintf(stderr, "Unable to write data to the in_message pipe: %zu\n", in_message_pipe_write_res);
  }
  */
}

static input_dev_t in_iio_dev = {
  .dev_type = input_dev_type_iio,
  .filters = {
    .iio = {
      .name = "bmi323",
    }
  },
  //.logic = &global_logic,
  //.input_filter_fn = input_filter_imu_identity,
};

static input_dev_t in_asus_kb_1_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .ev_input_map_fn = asus_kbd_ev_map,
};

static input_dev_t in_asus_kb_2_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .ev_input_map_fn = asus_kbd_ev_map,
};

static input_dev_t in_asus_kb_3_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .ev_input_map_fn = asus_kbd_ev_map,
};

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Microsoft X-Box 360 pad"
    }
  },
  .ev_input_map_fn = xbox360_ev_map,
};

input_dev_t in_devs[] = {
  
};

input_dev_composite_t rc71l_composite = {
  .dev = {
    &in_xbox_dev,
    &in_iio_dev,
    &in_asus_kb_1_dev,
    &in_asus_kb_2_dev,
    &in_asus_kb_3_dev,
  },
  .dev_count = 5,
};

input_dev_composite_t* rog_ally_device_def(const controller_settings_t *const settings) {
  in_xbox_dev.user_data = (void*)settings;
  return &rc71l_composite;
}
