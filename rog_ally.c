#include "rog_ally.h"
#include "input_dev.h"
#include "dev_hidraw.h"
#include "message.h"
#include "xbox360.h"
#include <linux/input-event-codes.h>
#include <stdio.h>

typedef enum rc71l_platform_mode {
  rc71l_platform_mode_hidraw,
  rc71l_platform_mode_linux_and_asusctl,
} rc71l_platform_mode_t;

typedef struct rc71l_platform {
  rc71l_platform_mode_t platform_mode;
  
  union {
    dev_hidraw_t *hidraw;
  } platform;

  unsigned long mode;
  unsigned int modes_count;
} rc71l_platform_t;

static rc71l_platform_t hw_platform;

typedef struct rc71l_asus_kbd_user_data {
	struct udev *udev;
} rc71l_asus_kbd_user_data_t;

static rc71l_asus_kbd_user_data_t asus_userdata;

static char* find_kernel_sysfs_device_path(struct udev *udev) {
    struct udev_enumerate *const enumerate = udev_enumerate_new(udev);
    if (enumerate == NULL) {
        fprintf(stderr, "Error in udev_enumerate_new: mode switch will not be available.\n");
        return NULL;
    }

    const int add_match_subsystem_res = udev_enumerate_add_match_subsystem(enumerate, "hid");
    if (add_match_subsystem_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_subsystem: %d\n", add_match_subsystem_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    const int add_match_sysattr_res = udev_enumerate_add_match_sysattr(enumerate, "gamepad_mode", NULL);
    if (add_match_sysattr_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_sysattr: %d\n", add_match_sysattr_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    const int enumerate_scan_devices_res = udev_enumerate_scan_devices(enumerate);
    if (enumerate_scan_devices_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_scan_devices: %d\n", enumerate_scan_devices_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    struct udev_list_entry *const udev_lst_frst = udev_enumerate_get_list_entry(enumerate);

    struct udev_list_entry *list_entry = NULL;
    udev_list_entry_foreach(list_entry, udev_lst_frst) {
        const char* const name = udev_list_entry_get_name(list_entry);

        const unsigned long len = strlen(name) + 1;
        char *const result = malloc(len);
        memset(result, 0, len);
        strncat(result, name, len - 1);

        udev_enumerate_unref(enumerate);

        return result;
    }

    udev_enumerate_unref(enumerate);
    return NULL;
}

static int get_next_mode(int current_mode) {
	if (current_mode == 1)
		return 2;
	if (current_mode == 2)
		return 3;
	if (current_mode == 3)
		return 1;
	else
		fprintf(stderr, "Invalid current mode: %d -- 1 (gamepad) will be set\n", current_mode);

	return 1;
}

static int asus_kbd_ev_map(
	const dev_in_settings_t *const conf,
	const evdev_collected_t *const e,
	in_message_t *const messages,
	size_t messages_len,
	void* user_data
) {
	rc71l_asus_kbd_user_data_t *const asus_kbd_user_data = (rc71l_asus_kbd_user_data_t*)user_data;
	int written_msg = 0;

	for (size_t i = 0; i < e->ev_count; ++i) {
		if (e->ev[i].type == EV_KEY) {
			if (e->ev[i].code == KEY_F14) {
				// this is left back paddle, works as expected
				const in_message_t current_message = {
					.type = GAMEPAD_SET_ELEMENT,
					.data = {
						.gamepad_set = {
							.element = GAMEPAD_BTN_L5,
							.status = {
								.btn = e->ev[1].value,
							}
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_F15) {
				// this is right back paddle, works as expected
				const in_message_t current_message = {
					.type = GAMEPAD_SET_ELEMENT,
					.data = {
						.gamepad_set = {
							.element = GAMEPAD_BTN_R5,
							.status = {
								.btn = e->ev[1].value,
							}
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if ((e->ev[i].code == KEY_F16) && (e->ev[i].value != 0)) {
				// this is left screen button, on release both 0 and 1 events are emitted so just discard the 0
				const in_message_t current_message = {
					.type = GAMEPAD_ACTION,
					.data = {
						.action = GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER,
					}
				};

				messages[written_msg++] = current_message;
			} else if ((e->ev[i].code == KEY_PROG1) && (e->ev[i].value != 0)) {
				// this is right screen button, on short release both 0 and 1 events are emitted so just discard the 0
				if (conf->enable_qam) {
					const in_message_t current_message = {
						.type = GAMEPAD_ACTION,
						.data = {
							.action = GAMEPAD_ACTION_OPEN_STEAM_QAM,
						}
					};

					messages[written_msg++] = current_message;
				}
			} else if ((e->ev[i].code == KEY_F17) && (e->ev[i].value != 0)) {
				// this is right screen button, on long release, after passing short threshold both 0 and 1 events are emitted so just discard the 0

			} else if ((e->ev[i].code == KEY_F18) && (e->ev[i].value != 0)) {
				// this is right screen button, on release both 0 and 1 events are emitted so just discard the 0

				// change controller mode
				if (asus_kbd_user_data == NULL) {
					fprintf(stderr, "asus keyboard user data unavailable -- skipping mode change\n");
					continue;
				}

				char *const kernel_sysfs = find_kernel_sysfs_device_path(asus_kbd_user_data->udev);
				if (kernel_sysfs == NULL) {
					fprintf(stderr, "Kernel interface to Asus MCU not found -- skipping mode change\n");
					continue;
				}

				printf("Asus MCU kernel interface found at %s -- switching mode\n", kernel_sysfs);

				const size_t tmp_path_max_len = strlen(kernel_sysfs) + 256;
				char *tmp_path = malloc(tmp_path_max_len);

				if (tmp_path != NULL) {
					memset(tmp_path, 0, tmp_path_max_len);
					snprintf(tmp_path, tmp_path_max_len - 1, "%s/gamepad_mode", kernel_sysfs);

					int gamepad_mode_fd = open(tmp_path, O_RDONLY);
					if (gamepad_mode_fd > 0) {
						char current_mode_str[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
						int current_mode_read_res = read(gamepad_mode_fd, (void*)current_mode_str, sizeof(current_mode_str));
						if (current_mode_read_res > 0) {
							close(gamepad_mode_fd);

							int current_mode;
							sscanf(current_mode_str, "%d", &current_mode);

							const int new_mode = get_next_mode(current_mode);
							printf("Current mode is set to %d (read from %s) -- switching to %d", current_mode, current_mode_str, new_mode);

							char new_mode_str[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							snprintf(new_mode_str, sizeof(new_mode_str) - 1, "%d", new_mode);
							gamepad_mode_fd = open(tmp_path, O_WRONLY);
							if (gamepad_mode_fd > 0) {
								if (write(gamepad_mode_fd, new_mode_str, strlen(new_mode_str)) > 0) {
									printf("Controller mode switched successfully to %s\n", new_mode_str);
								} else {
									fprintf(stderr, "Unable to switch controller mode to %s: %d -- expect bugs\n", new_mode_str, errno);
								}
								close(gamepad_mode_fd);
							} else {
								fprintf(stderr, "Unable to open gamepad mode file to switch mode: %d\n", errno);
							}
						} else {
							close(gamepad_mode_fd);
							fprintf(stderr, "Unable to read gamepad_mode file to get current mode: %d", errno);
						}
					} else {
						fprintf(stderr, "Unable to open gamepad_mode file in read-only mode to get current mode: %d", errno);
					}

					free(tmp_path);
				} else {
					fprintf(stderr, "Unable to allocate enough memory\n");
				}

				free(kernel_sysfs);
			} else if (e->ev[i].code == BTN_LEFT) {
				const in_message_t current_message = {
					.type = MOUSE_EVENT,
					.data = {
						.mouse_event = {
							.type = MOUSE_BTN_LEFT,
							.value = e->ev[i].value,
						}
					}
				};

				messages[written_msg++] = current_message;
			}  else if (e->ev[i].code == BTN_MIDDLE) {
				const in_message_t current_message = {
					.type = MOUSE_EVENT,
					.data = {
						.mouse_event = {
							.type = MOUSE_BTN_MIDDLE,
							.value = e->ev[i].value,
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == BTN_RIGHT) {
				const in_message_t current_message = {
					.type = MOUSE_EVENT,
					.data = {
						.mouse_event = {
							.type = MOUSE_BTN_RIGHT,
							.value = e->ev[i].value,
						}
					}
				};

				messages[written_msg++] = current_message;
			}
		} else if (e->ev[i].type == EV_REL) {
			if (e->ev[i].code == REL_X) {
				const in_message_t current_message = {
					.type = MOUSE_EVENT,
					.data = {
						.mouse_event = {
							.type = MOUSE_ELEMENT_X,
							.value = e->ev[i].value,
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == REL_Y) {
				const in_message_t current_message = {
					.type = MOUSE_EVENT,
					.data = {
						.mouse_event = {
							.type = MOUSE_ELEMENT_Y,
							.value = e->ev[i].value,
						}
					}
				};

				messages[written_msg++] = current_message;
			}
		}
	}
	
	return written_msg;
}

static hidraw_filters_t n_key_hidraw_filters = {
  .pid = 0x1abe,
  .vid = 0x0b05,
  .rdesc_size = 167, // 48 83 167
};

static input_dev_t in_iio_dev = {
  .dev_type = input_dev_type_iio,
  .filters = {
    .iio = {
      .name = "bmi323-imu",
    }
  },
  .map = {
	.iio_settings = {
		.sampling_freq_hz = "1600.000",
		.post_matrix =
				/*
				// this is the testing but "wrong" mount matrix
				{
					{0.0f, 0.0f, -1.0f},
					{0.0f, 1.0f, 0.0f},
					{-1.0f, 0.0f, 0.0f}
				};
				*/
				// this is the correct matrix:
				{
					{-1, 0, 0},
					{0, 1, 0},
					{0, 0, -1}
				},
	}
  }
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
  .user_data = (void*)&asus_userdata,
  .map = {
	.ev_input_map_fn = asus_kbd_ev_map,
  }
};

static input_dev_t in_asus_kb_2_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .user_data = (void*)&asus_userdata,
  .map = {
	.ev_input_map_fn = asus_kbd_ev_map,
  }
};

static input_dev_t in_asus_kb_3_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .user_data = &asus_userdata,
  .map = {
	.ev_input_map_fn = asus_kbd_ev_map,
  }
};

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Microsoft X-Box 360 pad"
    }
  },
  .map = {
	.ev_input_map_fn = xbox360_ev_map,
  }
};

enum rc71l_leds_mode {
  ROG_ALLY_MODE_STATIC           = 0,
  ROG_ALLY_MODE_BREATHING        = 1,
  ROG_ALLY_MODE_COLOR_CYCLE      = 2,
  ROG_ALLY_MODE_RAINBOW          = 3,
  ROG_ALLY_MODE_STROBING         = 10,
  ROG_ALLY_MODE_DIRECT           = 0xFF,
} rc71l_leds_mode_t;

enum rc71l_leds_speed {
    ROG_ALLY_SPEED_MIN              = 0xE1,
    ROG_ALLY_SPEED_MED              = 0xEB,
    ROG_ALLY_SPEED_MAX              = 0xF5
} rc71l_leds_speed_t;

enum rc71l_leds_direction {
    ROG_ALLY_DIRECTION_RIGHT        = 0x00,
    ROG_ALLY_DIRECTION_LEFT         = 0x01
} rc71l_leds_direction_t;

static int rc71l_platform_init(const dev_in_settings_t *const conf, void** platform_data) {
	int res = -EINVAL;

	rc71l_platform_t *const platform = &hw_platform;
	if (platform == NULL) {
		fprintf(stderr, "Unable to setup platform\n");
		res = -ENOMEM;
		goto rc71l_platform_init_err;
	}

	*platform_data = (void*)platform;

	// setup asus keyboard(s) user_data
	asus_userdata.udev = udev_new();

	res = dev_hidraw_open(&n_key_hidraw_filters, &platform->platform.hidraw);
	if (res != 0) {
		fprintf(stderr, "Unable to open the ROG ally hidraw main device...\n");
		free(*platform_data);
		*platform_data = NULL;
		goto rc71l_platform_init_err;
	}

	platform->platform_mode = rc71l_platform_mode_hidraw;

	const int fd = dev_hidraw_get_fd(platform->platform.hidraw);
	
	platform->mode = 0;
	platform->modes_count = 2;
	printf("ROG Ally platform will be managed over hidraw. I'm sorry fluke.\n");

rc71l_platform_init_err:
	return res;
}

static void rc71l_platform_deinit(const dev_in_settings_t *const conf, void** platform_data) {
	rc71l_platform_t *const platform = (rc71l_platform_t *)(*platform_data);

	udev_unref(asus_userdata.udev);

	if ((platform->platform_mode == rc71l_platform_mode_hidraw) && (platform->platform.hidraw != NULL)) {
		dev_hidraw_close(platform->platform.hidraw);
		platform->platform.hidraw = NULL;
	}

	*platform_data = NULL;
}

static int rc71l_platform_leds(const dev_in_settings_t *const conf, uint8_t r, uint8_t g, uint8_t b, void* platform_data) {
	rc71l_platform_t *const platform = (rc71l_platform_t *)platform_data;
	if (platform == NULL) {
		return -EINVAL;
	}

	const uint8_t brightness_buf[] = {
		0x5A, 0xBA, 0xC5, 0xC4, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t colors_buf[] = {
		0x5A, 0xB3, 0x00, ROG_ALLY_MODE_STATIC, r, g, b, 0x00, ROG_ALLY_DIRECTION_RIGHT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if ((platform->platform_mode == rc71l_platform_mode_hidraw) && (platform->platform.hidraw != NULL)) {
		int fd = dev_hidraw_get_fd(platform->platform.hidraw);

		if (write(fd, brightness_buf, sizeof(brightness_buf)) != 64) {
			fprintf(stderr, "Unable to send LEDs brightness (1) command change -- attempt to reacquire hidraw\n");

			dev_hidraw_close(platform->platform.hidraw);

			const int reopen_res = dev_hidraw_open(&n_key_hidraw_filters, &platform->platform.hidraw);
			if (reopen_res != 0) {
				fprintf(stderr, "Unable to (re)open the ROG ally hidraw main device...\n");
				goto rc71l_platform_leds_err;
			} else {
				printf("ROG ally hidraw main device reacquired\n");
				fd = dev_hidraw_get_fd(platform->platform.hidraw);

				if (write(fd, brightness_buf, sizeof(brightness_buf)) != 64) {
					fprintf(stderr, "Unable to send LEDs brightness (1) command change after hidraw reset -- giving up\n");
					goto rc71l_platform_leds_err;
				}
			}
		}

		if (write(fd, colors_buf, sizeof(colors_buf)) != 64) {
			fprintf(stderr, "Unable to send LEDs color command change (1)\n");
			goto rc71l_platform_leds_err;
		}

		return 0;
	}

rc71l_platform_leds_err:
  return -EINVAL;
}

input_dev_composite_t rc71l_composite = {
  .dev = {
    &in_xbox_dev,
    &in_iio_dev,
    &in_asus_kb_1_dev,
    &in_asus_kb_2_dev,
    &in_asus_kb_3_dev,
  },
  .dev_count = 5,
  .init_fn = rc71l_platform_init,
  .deinit_fn = rc71l_platform_deinit,
  .leds_fn = rc71l_platform_leds,
};

input_dev_composite_t* rog_ally_device_def(void) {
	return &rc71l_composite;
}
