#include "rog_ally.h"
#include "input_dev.h"
#include "dev_hidraw.h"
#include "message.h"
#include "xbox360.h"
#include <stdio.h>

static const char iio_base_path[] = "/sys/bus/iio/devices/iio:device0/";

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

struct rc71l_platform;

typedef struct rc71l_xbox360_user_data {
	struct rc71l_platform* parent;

	uint64_t accounted_mode_switches;

	uint64_t mode_switched;

	uint64_t timeout_after_mode_switch;

	bool mode_switch_rumbling;

	struct ff_effect mode_switch_rumble_effect;

} rc71l_xbox360_user_data_t;

typedef struct rc71l_asus_kbd_user_data {
	struct rc71l_platform* parent;

	struct udev *udev;

	int m1, m2;
} rc71l_asus_kbd_user_data_t;

typedef struct rc71l_asus_hidraw_user_data {
	struct rc71l_platform* parent;

} rc71l_asus_hidraw_user_data_t;

typedef struct rc71l_timer_user_data {
	struct rc71l_platform* parent;

} rc71l_timer_user_data_t;

rc71l_timer_user_data_t timer_user_data;

typedef struct rc71l_platform {
	rc71l_asus_kbd_user_data_t* kbd_user_data;

	rc71l_xbox360_user_data_t* xbox360_user_data;

	rc71l_timer_user_data_t* timer_data;

	rc71l_asus_hidraw_user_data_t* hidraw_user_data;

	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	} static_led_color;

	uint64_t thermal_profile_expired;
	size_t current_thermal_profile;
	size_t next_thermal_profile;

} rc71l_platform_t;

static rc71l_asus_hidraw_user_data_t hidraw_userdata = {
	.parent = NULL,
};

static rc71l_asus_kbd_user_data_t asus_userdata = {
	.parent = NULL,
	.udev = NULL,
	.m1 = 0,
	.m2 = 0,
};

static rc71l_xbox360_user_data_t controller_user_data = {
	.accounted_mode_switches = 0,
	.mode_switched = 0,
	.timeout_after_mode_switch = 0,
	.mode_switch_rumbling = false,
	.mode_switch_rumble_effect = {
		.type = FF_RUMBLE,
		.id = -1,
		.replay = {
			.delay = 0x00,
			.length = 50
		},
		.u = {
			.rumble = {
				.strong_magnitude = 0xFFFF,
				.weak_magnitude = 0xFFFF,
			}
		}
	}
};

static rc71l_platform_t hw_platform = {
	.kbd_user_data = &asus_userdata,
	.xbox360_user_data = &controller_user_data,
	.timer_data = &timer_user_data,
	.hidraw_user_data = &hidraw_userdata,
	.static_led_color = {
		.r = 0x40,
		.g = 0x40,
		.b = 0x40,
	},
	.current_thermal_profile = 0,
	.next_thermal_profile = 0,
};

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
			if (e->ev[i].value > 1) {
				continue;
			}

			if (e->ev[i].code == KEY_F14) {
				// this is left back paddle, works as expected

				if (e->ev[i].value == 0) {
					asus_kbd_user_data->m1 -= (asus_kbd_user_data->m1 == 0) ? 0 : 1;
				} else if (e->ev[i].value == 1) {
					asus_kbd_user_data->m1 += 1;
				}

				if (conf->m1m2_mode == 0) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_L4,
								.status = {
									.btn = e->ev[1].value,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				} else if (conf->m1m2_mode == 1) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_TOUCHPAD,
								.status = {
									.btn = (asus_kbd_user_data->m1 + asus_kbd_user_data->m2) == 0 ? 0 : 1,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				} else if (conf->m1m2_mode == 2) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_JOIN_LEFT_ANALOG_AND_GYROSCOPE,
								.status = {
									.btn = e->ev[1].value,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				}
			} else if (e->ev[i].code == KEY_F15) {
				// this is right back paddle, works as expected

				if (e->ev[i].value == 0) {
					asus_kbd_user_data->m2 -= (asus_kbd_user_data->m2 == 0) ? 0 : 1;
				} else if (e->ev[i].value == 1) {
					asus_kbd_user_data->m2 += 1;
				}

				if (conf->m1m2_mode == 0) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_R4,
								.status = {
									.btn = e->ev[1].value,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				} else if (conf->m1m2_mode == 1) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_TOUCHPAD,
								.status = {
									.btn = (asus_kbd_user_data->m1 + asus_kbd_user_data->m2) == 0 ? 0 : 1,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				} else if (conf->m1m2_mode == 2) {
					const in_message_t current_message = {
						.type = GAMEPAD_SET_ELEMENT,
						.data = {
							.gamepad_set = {
								.element = GAMEPAD_BTN_JOIN_RIGHT_ANALOG_AND_GYROSCOPE,
								.status = {
									.btn = e->ev[1].value,
								}
							}
						}
					};

					messages[written_msg++] = current_message;
				}
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
			} else if ((e->ev[i].code == KEY_F18) && (e->ev[i].value != 0)) {
				// this is right screen button, on long release both 0 and 1 events are emitted so just discard the 0

			} else if ((e->ev[i].code == KEY_DELETE) && (e->ev[i].value != 0)) {
				// this is left screen button, on long release both 0 and 1 events are emitted so just discard the 0

				if (conf->enable_thermal_profiles_switching) {
					asus_kbd_user_data->parent->next_thermal_profile = asus_kbd_user_data->parent->current_thermal_profile + 1;

					printf("Requested switch to thermal profile %d\n", (int)asus_kbd_user_data->parent->next_thermal_profile);
				}
			} else if ((e->ev[i].code == KEY_F17) && (e->ev[i].value != 0)) {
				// this is right screen button, on long press, after passing short threshold both 0 and 1 events are emitted so just discard the 0

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
							printf("Current mode is set to %d (read from %s) -- switching to %d\n", current_mode, current_mode_str, new_mode);

							char new_mode_str[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							snprintf(new_mode_str, sizeof(new_mode_str) - 1, "%d", new_mode);
							gamepad_mode_fd = open(tmp_path, O_WRONLY);
							if (gamepad_mode_fd > 0) {
								if (write(gamepad_mode_fd, new_mode_str, strlen(new_mode_str)) > 0) {
									printf("Controller mode switched successfully to %s\n", new_mode_str);

									asus_kbd_user_data->parent->xbox360_user_data->mode_switched++;
								} else {
									fprintf(stderr, "Unable to switch controller mode to %s: %d -- expect bugs\n", new_mode_str, errno);
								}
								close(gamepad_mode_fd);
							} else {
								fprintf(stderr, "Unable to open gamepad mode file to switch mode: %d\n", errno);
							}
						} else {
							close(gamepad_mode_fd);
							fprintf(stderr, "Unable to read gamepad_mode file to get current mode: %d\n", errno);
						}
					} else {
						fprintf(stderr, "Unable to open gamepad_mode file in read-only mode to get current mode: %d\n", errno);
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
			} else if (e->ev[i].code == KEY_LEFTCTRL) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_LCRTL,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_Q) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_Q,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_W) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_W,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_E) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_E,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_R) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_R,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_T) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_T,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_Y) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_Y,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_U) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_U,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_I) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_I,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_O) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_O,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_P) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_P,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_A) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_A,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_S) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_S,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_D) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_D,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_F) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_F,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_G) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_G,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_H) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_H,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_J) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_J,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_K) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_K,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_L) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_L,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_Z) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_Z,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_X) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_X,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_C) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_C,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_V) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_V,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_B) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_B,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_N) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_N,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_M) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_M,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_0) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_0,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_1) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_1,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_2) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_2,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_3) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_3,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_4) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_4,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_5) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_5,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_6) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_6,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_7) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_7,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_8) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_8,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_9) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_NUM_9,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_UP) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_UP,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_DOWN) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_DOWN,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_LEFT) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_LEFT,
							.value = e->ev[i].value
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == KEY_RIGHT) {
				const in_message_t current_message = {
					.type = KEYBOARD_SET_ELEMENT,
					.data = {
						.kbd_set = {
							.type = KEYBOARD_KEY_RIGHT,
							.value = e->ev[i].value
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

static void rc71l_timer_touchscreen(
	const dev_in_settings_t *const conf,
	struct libevdev* evdev,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
) {
	
}

static int touchscreen_ev_map(
	const dev_in_settings_t *const conf,
	const evdev_collected_t *const e,
	in_message_t *const messages,
	size_t messages_len,
	void* user_data
) {
	int written_msg = 0;

	for (size_t i = 0; i < e->ev_count; ++i) {
		if (e->ev[i].type == EV_ABS) {
			if (e->ev[i].code == ABS_MT_TRACKING_ID) {
				const in_message_t current_message = {
					.type = GAMEPAD_SET_ELEMENT,
					.data = {
						.gamepad_set = {
							.element = GAMEPAD_TOUCHPAD_TOUCH_ACTIVE,
							.status = {
								.touchpad_active = {
									.status = e->ev[i].value,
								}
							}
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == ABS_MT_POSITION_X) {
				const in_message_t current_message = {
					.type = GAMEPAD_SET_ELEMENT,
					.data = {
						.gamepad_set = {
							.element = GAMEPAD_TOUCHPAD_X,
							.status = {
								.touchpad_x = {
									.value = e->ev[i].value,
								}
							}
						}
					}
				};

				messages[written_msg++] = current_message;
			} else if (e->ev[i].code == ABS_MT_POSITION_Y) {
				const in_message_t current_message = {
					.type = GAMEPAD_SET_ELEMENT,
					.data = {
						.gamepad_set = {
							.element = GAMEPAD_TOUCHPAD_Y,
							.status = {
								.touchpad_y = {
									.value = e->ev[i].value,
								}
							}
						}
					}
				};

				messages[written_msg++] = current_message;
			}
		}
	}

	return written_msg;
}

static input_dev_t in_touchscreen_dev = {
	.dev_type = input_dev_type_uinput,
	.filters = {
		.ev = {
			.name = "NVTK0603:00 0603:F200"
		}
	},
	.user_data = NULL,
	.map = {
		.ev_callbacks = {
			.input_map_fn = touchscreen_ev_map,
			.timeout_callback = rc71l_timer_touchscreen,
		},
	}
};

static void rc71l_timer_asus_kbd(
	const dev_in_settings_t *const conf,
	struct libevdev* evdev,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
) {

}

static input_dev_t in_asus_kb_1_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  .user_data = (void*)&asus_userdata,
  .map = {
	.ev_callbacks = {
		.input_map_fn = asus_kbd_ev_map,
		.timeout_callback = rc71l_timer_asus_kbd,
	},
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
	.ev_callbacks = {
		.input_map_fn = asus_kbd_ev_map,
		.timeout_callback = rc71l_timer_asus_kbd,
	},
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
	.ev_callbacks = {
		.input_map_fn = asus_kbd_ev_map,
		.timeout_callback = rc71l_timer_asus_kbd,
	},
  }
};

static void rc71l_timer_xbox360(
	const dev_in_settings_t *const conf,
	struct libevdev* evdev,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
) {
	if (strcmp(timer_name, "RC71L_timer") != 0) {
		return;
	}

	rc71l_xbox360_user_data_t *const xbox360_data = (rc71l_xbox360_user_data_t*)user_data;	

	if (conf->rumble_on_mode_switch) {
		if (xbox360_data->accounted_mode_switches != xbox360_data->mode_switched) {
			const int fd = libevdev_get_fd(evdev);

			xbox360_data->accounted_mode_switches = xbox360_data->mode_switched;

			xbox360_data->mode_switch_rumbling = true;

			// load the new effect data
			xbox360_data->mode_switch_rumble_effect.u.rumble.strong_magnitude = 0xFFFF;
			xbox360_data->mode_switch_rumble_effect.u.rumble.weak_magnitude = 0xFFFF;

			// upload the new effect to the device
			const int effect_upload_res = ioctl(fd, EVIOCSFF, &xbox360_data->mode_switch_rumble_effect);
			if (effect_upload_res == 0) {
				struct input_event rumble_stop = {
					.type = EV_FF,
					.code = xbox360_data->mode_switch_rumble_effect.id,
					.value = 0,
				};

				const int rumble_stop_res = write(fd, (const void*) &rumble_stop, sizeof(rumble_stop));
				if (rumble_stop_res != sizeof(rumble_stop)) {
					fprintf(stderr, "Unable to stop the previous rumble: %d\n", rumble_stop_res);
				}

				const struct input_event rumble_play = {
					.type = EV_FF,
					.code = xbox360_data->mode_switch_rumble_effect.id,
					.value = 1,
				};

				const int effect_start_res = write(fd, (const void*)&rumble_play, sizeof(rumble_play));
				if (effect_start_res != sizeof(rumble_play)) {
					fprintf(stderr, "Unable to write input event starting the mode-switch rumble: %d\n", effect_start_res);
				}
			} else {
				fprintf(stderr, "Unable to update force-feedback effect for mode-switch rumble: %d\n", effect_upload_res);

				xbox360_data->mode_switch_rumble_effect.id = -1;
			}
		} else if (xbox360_data->mode_switch_rumbling) {
			const int fd = libevdev_get_fd(evdev);

			xbox360_data->timeout_after_mode_switch++;
			
			if (xbox360_data->timeout_after_mode_switch >= 10) {
				xbox360_data->mode_switch_rumbling = false;

				if (xbox360_data->mode_switch_rumble_effect.id != -1) {
					/*
					struct input_event rumble_stop = {
						.type = EV_FF,
						.code = xbox360_data->mode_switch_rumble_effect.id,
						.value = 0,
					};

					const int rumble_stop_res = write(fd, (const void*) &rumble_stop, sizeof(rumble_stop));
					if (rumble_stop_res != sizeof(rumble_stop)) {
						fprintf(stderr, "Unable to stop the previous rumble: %d\n", rumble_stop_res);
					}
					*/
				}
			}
		}
	}
}

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Microsoft X-Box 360 pad"
    }
  },
  .user_data = &controller_user_data,
  .map = {
	.ev_callbacks = {
		.input_map_fn = xbox360_ev_map,
		.timeout_callback = rc71l_timer_xbox360,
	},
  }
};

static int rc71l_hidraw_map(const dev_in_settings_t *const conf, int hidraw_fd, in_message_t *const messages, size_t messages_len, void* user_data) {
	uint8_t data[256];
	const int read_res = read(hidraw_fd, data, sizeof(data));

	if (read_res < 0) {
		return -EIO;
	}
	
	//printf("Got %d bytes from Asus MCU\n", read_res); // either 6 or 32

	return 0;
}

static int rc71l_hidraw_rumble(const dev_in_settings_t *const conf, int hidraw_fd, uint8_t left_motor, uint8_t right_motor, void* user_data) {
	return 0;
}

static int rc71l_hidraw_set_leds_inner(int hidraw_fd, uint8_t r, uint8_t g, uint8_t b) {
	const uint8_t colors_buf[] = {
		0x5A, 0xB3, 0x00, ROG_ALLY_MODE_STATIC, r, g, b, 0x00, ROG_ALLY_DIRECTION_RIGHT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (write(hidraw_fd, colors_buf, sizeof(colors_buf)) != 64) {
		fprintf(stderr, "Unable to send LEDs color command change (1)\n");
		goto rc71l_hidraw_set_leds_inner_err;
	}

	return 0;

rc71l_hidraw_set_leds_inner_err:
  return -EIO;
}

static int rc71l_hidraw_set_leds(const dev_in_settings_t *const conf, int hidraw_fd, uint8_t r, uint8_t g, uint8_t b, void* user_data) {
	rc71l_asus_hidraw_user_data_t *const hidraw_data = (rc71l_asus_hidraw_user_data_t*)user_data;
	if (hidraw_data == NULL) {
		return -ENOENT;
	}

	if (
		(hidraw_data->parent->static_led_color.r == r) &&
		(hidraw_data->parent->static_led_color.g == g) &&
		(hidraw_data->parent->static_led_color.b == b)
	) {
		return 0;
	}

	hidraw_data->parent->static_led_color.r = r;
	hidraw_data->parent->static_led_color.g = g;
	hidraw_data->parent->static_led_color.b = b;

	const int res = conf->enable_leds_commands ? rc71l_hidraw_set_leds_inner(
		hidraw_fd,
		hidraw_data->parent->static_led_color.r,
		hidraw_data->parent->static_led_color.g,
		hidraw_data->parent->static_led_color.b
	) : 0;

	if (res != 0) {
		fprintf(stderr, "Error setting leds: %d\n", res);
	}

	return res;
}

#define PROFILES_COUNT 4

static const char* epp[PROFILES_COUNT] = {
	"/bin/bash -c 'shopt -s nullglob; echo \"power\" | tee /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference'",
	"/bin/bash -c 'shopt -s nullglob; echo \"balance_performance\" | tee /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference'",
	"/bin/bash -c 'shopt -s nullglob; echo \"balance_power\" | tee /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference'",
	"/bin/bash -c 'shopt -s nullglob; echo \"performance\" | tee /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference'"
};

static const char* profiles[PROFILES_COUNT] = {
	"asusctl profile -P Quiet",
	"asusctl profile -P Balanced",
	"asusctl profile -P Performance",
	"asusctl profile -P Performance",
};

static const struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} colors[PROFILES_COUNT] = {
	{
		.r = 0x00, // Blue
		.g = 0x00,
		.b = 0xFF,
	},
	{
		.r = 0x00, // Green
		.g = 0xFF,
		.b = 0x00,
	},
	{
		.r = 0xFF, // Almost-orange/yellow
		.g = 0xBF,
		.b = 0x00,
	},
	{
		.r = 0xFF, // Red
		.g = 0x00,
		.b = 0x00,
	}
};

static void rc71l_hidraw_timer(
    const dev_in_settings_t *const conf,
    int hidraw_fd,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
) {
	// one tick is 60ms, avoid all other timers
	if (strcmp(timer_name, "RC71L_timer") != 0) {
		return;
	}

	rc71l_asus_hidraw_user_data_t *const hidraw_data = (rc71l_asus_hidraw_user_data_t*)user_data;
	if (hidraw_data == NULL) {
		return;
	}

	if (conf->enable_thermal_profiles_switching) {
		if (hidraw_data->parent->current_thermal_profile != hidraw_data->parent->next_thermal_profile) {
			if (hidraw_data->parent->thermal_profile_expired == 0) {
				++hidraw_data->parent->thermal_profile_expired;
				uint64_t thermal_profile_index = hidraw_data->parent->next_thermal_profile % PROFILES_COUNT;

				const int change_thermal_result = system(profiles[thermal_profile_index]);
				if (change_thermal_result == 0) {
					const int change_amd_pstate_epp = system(epp[thermal_profile_index]);
					if (change_amd_pstate_epp == 0) {
						const int leds_set = rc71l_hidraw_set_leds_inner(
							hidraw_fd,
							colors[thermal_profile_index].r,
							colors[thermal_profile_index].g,
							colors[thermal_profile_index].b
						);

						if (leds_set != 0) {
							fprintf(stderr, "Error setting leds to tell the user about the new profile: %d\n", leds_set);
						}
					} else {
						fprintf(
							stderr,
							"Error setting the new AMD P-State hint with '%s': %d\n",
							epp[thermal_profile_index],
							change_amd_pstate_epp
						);
					}
				} else {
					fprintf(
						stderr,
						"Error setting the new thermal profile with '%s': %d\n",
						profiles[thermal_profile_index],
						change_thermal_result
					);
				}
			} else {
				++hidraw_data->parent->thermal_profile_expired;

				if (hidraw_data->parent->thermal_profile_expired > 18) {
					hidraw_data->parent->current_thermal_profile = hidraw_data->parent->next_thermal_profile;
					hidraw_data->parent->thermal_profile_expired = 0;
					rc71l_hidraw_set_leds_inner(
						hidraw_fd,
						hidraw_data->parent->static_led_color.r,
						hidraw_data->parent->static_led_color.g,
						hidraw_data->parent->static_led_color.b
					);
				}
			}
		}
	}
}

static input_dev_t nkey_dev = {
	.dev_type = input_dev_type_hidraw,
	.filters = {
		.hidraw = {
			.pid = 0x1abe,
			.vid = 0x0b05,
			.rdesc_size = 167, // 48 83 167
		}
	},
	.user_data = (void*)&hidraw_userdata,
	.map = {
		.hidraw_callbacks = {
			.leds_callback = rc71l_hidraw_set_leds,
			.rumble_callback = rc71l_hidraw_rumble,
			.map_callback = rc71l_hidraw_map,
			.timeout_callback = rc71l_hidraw_timer,
		}
	}
};

static int rc71l_platform_init(const dev_in_settings_t *const conf, void** platform_data) {
	int res = -EINVAL;

	rc71l_platform_t *const platform = &hw_platform;
	*platform_data = (void*)platform;

	// setup asus keyboard(s) user_data
	platform->kbd_user_data->parent = platform;
	platform->xbox360_user_data->parent = platform;
	platform->timer_data->parent = platform;
	platform->kbd_user_data->udev = udev_new();
	platform->hidraw_user_data->parent = platform;
	if (platform->kbd_user_data->udev == NULL) {
		fprintf(stderr, "Unable to initialize udev\n");
		res = -ENOMEM;
		goto rc71l_platform_init_err;
	}

	res = 0;

	if (conf->enable_leds_commands) {
		char command_str[64] = "\0";
		sprintf(
			command_str,
			"asusctl led-mode static -c %02X%02X%02X",
			platform->static_led_color.r,
			platform->static_led_color.g,
			platform->static_led_color.b
		);

		const int led_mode_cmd_result = system(command_str);
		if (led_mode_cmd_result != 0) {
			fprintf(
				stderr,
				"Error setting led mode to static over asusctl: %d\n",
				led_mode_cmd_result
			);

			res = led_mode_cmd_result;
			goto rc71l_platform_init_err;
		}

		memset(command_str, 0, sizeof(command_str));
		sprintf(command_str, "asusctl -k high");

		const int led_brightness_cmd_result = system(command_str);
		if (led_brightness_cmd_result != 0) {
			fprintf(
				stderr,
				"Error setting led brightness over asusctl: %d\n",
				led_brightness_cmd_result
			);

			res = led_brightness_cmd_result;
			goto rc71l_platform_init_err;
		}
	}

rc71l_platform_init_err:
	return res;
}

static void rc71l_platform_deinit(const dev_in_settings_t *const conf, void** platform_data) {
	rc71l_platform_t *const platform = (rc71l_platform_t *)(*platform_data);

	if (platform_data != NULL) {
		if (platform->kbd_user_data != NULL) {
			udev_unref(platform->kbd_user_data->udev);
		}
	}

	*platform_data = NULL;
}

static int rc71l_platform_leds(const dev_in_settings_t *const conf, uint8_t r, uint8_t g, uint8_t b, void* platform_data) {
	rc71l_platform_t *const platform = (rc71l_platform_t*)platform_data;

	// hidraw is used for now
	return 0;
}



typedef struct dev_iio {
    char* path;
    char* name;
    uint32_t flags;

    FILE* accel_x_fd;
    FILE* accel_y_fd;
    FILE* accel_z_fd;

    double accel_scale_x;
    double accel_scale_y;
    double accel_scale_z;

    FILE* anglvel_x_fd;
    FILE* anglvel_y_fd;
    FILE* anglvel_z_fd;

    double anglvel_scale_x;
    double anglvel_scale_y;
    double anglvel_scale_z;

    double temp_scale;
    
    double outer_temp_scale;

    double mount_matrix[3][3];

    double anglvel_sampling_rate_hz;
	double accel_sampling_rate_hz;
} dev_old_iio_t;


static char* read_file(const char* base_path, const char *file) {
    char* res = NULL;
    char* fdir = NULL;
    long len = 0;

    len = strlen(base_path) + strlen(file) + 1;
    fdir = malloc(len);
    if (fdir == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device path, device skipped.\n", len);
        goto read_file_err;
    }
    strcpy(fdir, base_path);
    strcat(fdir, file);

    if (access(fdir, F_OK) == 0) {
        FILE* fp = fopen(fdir, "r");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            len = ftell(fp);
            rewind(fp);

            len += 1;
            res = malloc(len);
            if (res != NULL) {
                unsigned long read_bytes = fread(res, 1, len, fp);
                printf("Read %lu bytes from file %s\n", read_bytes, fdir);
            } else {
                fprintf(stderr, "Cannot allocate %ld bytes for %s content.\n", len, fdir);
            }

            fclose(fp);
        } else {
            fprintf(stderr, "Cannot open file %s.\n", fdir);
        }
    } else {
        fprintf(stderr, "File %s does not exists.\n", fdir);
    }

    free(fdir);
    fdir = NULL;

read_file_err:
    return res;
}

static int inline_write_file(const char* base_path, const char *file, const void* buf, size_t buf_sz) {
    char* fdir = NULL;

    int res = 0;

    const size_t len = strlen(base_path) + strlen(file) + 1;
    fdir = malloc(len);
    if (fdir == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device path, device skipped.\n", len);
        goto inline_write_file_err;
    }
    strcpy(fdir, base_path);
    strcat(fdir, file);

    if (access(fdir, F_OK) == 0) {
        FILE* fp = fopen(fdir, "w");
        if (fp != NULL) {
            res = fwrite(buf, 1, buf_sz, fp);
            if (res >= buf_sz) {
                printf("Written %d bytes to file %s\n", res, fdir);
            } else {
                fprintf(stderr, "Cannot write to %s: %d.\n", fdir, res);
            }

            fclose(fp);
        } else {
            fprintf(stderr, "Cannot open file %s.\n", fdir);
        }
    } else {
        fprintf(stderr, "File %s does not exists.\n", fdir);
    }

    free(fdir);
    fdir = NULL;

inline_write_file_err:
    return res;
}

static dev_old_iio_t* dev_old_iio_create(const char* path) {
    dev_old_iio_t *iio = malloc(sizeof(dev_old_iio_t));
    if (iio == NULL) {
        return NULL;
    }

    iio->anglvel_x_fd = NULL;
    iio->anglvel_y_fd = NULL;
    iio->anglvel_z_fd = NULL;
    iio->accel_x_fd = NULL;
    iio->accel_y_fd = NULL;
    iio->accel_z_fd = NULL;

    iio->accel_scale_x = 0.0f;
    iio->accel_scale_y = 0.0f;
    iio->accel_scale_z = 0.0f;
    iio->anglvel_scale_x = 0.0f;
    iio->anglvel_scale_y = 0.0f;
    iio->anglvel_scale_z = 0.0f;
    iio->temp_scale = 0.0f;

    iio->outer_temp_scale = 0.0;

    double mm[3][3] = 
		// this is the correct matrix:
		{
			{-1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0}
		};

    // store the mount matrix
    memcpy(iio->mount_matrix, mm, sizeof(mm));

    const long path_len = strlen(path) + 1;
    iio->path = malloc(path_len);
    if (iio->path == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device name, device skipped.\n", path_len);
        free(iio);
        iio = NULL;
        goto dev_old_iio_create_err;
    }
    strcpy(iio->path, path);

    // ============================================= DEVICE NAME ================================================
    iio->name = read_file(iio->path, "/name");
    if (iio->name == NULL) {
        fprintf(stderr, "Unable to read iio device name.\n");
        free(iio);
        iio = NULL;
        goto dev_old_iio_create_err;
    } else {
        int idx = strlen(iio->name) - 1;
        if ((iio->name[idx] == '\n') || ((iio->name[idx] == '\t'))) {
            iio->name[idx] = '\0';
        }
    }
    // ==========================================================================================================

    // ========================================== in_anglvel_scale ==============================================
    {
        const char* preferred_scale = LSB_PER_RAD_S_2000_DEG_S_STR;
        const char *scale_main_file = "/in_anglvel_scale";

        char* const anglvel_scale = read_file(iio->path, scale_main_file);
        if (anglvel_scale != NULL) {
            iio->anglvel_scale_x = iio->anglvel_scale_y = iio->anglvel_scale_z = strtod(anglvel_scale, NULL);
            free((void*)anglvel_scale);

            if (inline_write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->anglvel_scale_x = iio->anglvel_scale_y = iio->anglvel_scale_z = LSB_PER_RAD_S_2000_DEG_S;
                printf("anglvel scale changed to %f for device %s\n", iio->anglvel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_anglvel_scale for device %s.\n", iio->name);
            }
        } else {
            // TODO: what about if those are split in in_anglvel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_anglvel_scale from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_old_iio_create_err;
        }
    }
    // ==========================================================================================================

    // =========================================== in_accel_scale ===============================================
    {
        const char* preferred_scale = LSB_PER_16G_STR;
        const char *scale_main_file = "/in_accel_scale";

        char* const accel_scale = read_file(iio->path, scale_main_file);
        if (accel_scale != NULL) {
            iio->accel_scale_x = iio->accel_scale_y = iio->accel_scale_z = strtod(accel_scale, NULL);
            free((void*)accel_scale);

            if (inline_write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->accel_scale_x = iio->accel_scale_y = iio->accel_scale_z = LSB_PER_16G;
                printf("accel scale changed to %f for device %s\n", iio->accel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_accel_scale for device %s.\n", iio->name);
            }
        } else {
            // TODO: what about if those are plit in in_accel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_old_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================= temp_scale =================================================
    {
        char* const accel_scale = read_file(iio->path, "/in_temp_scale");
        if (accel_scale != NULL) {
            iio->temp_scale = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", iio->path, "/in_accel_scale");

            free(iio);
            iio = NULL;
            goto dev_old_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ======================================= anglvel_sampling_rate_hz ==========================================
    {
        const char* preferred_scale = PREFERRED_SAMPLING_FREQ_STR;
        const char *scale_main_file = "/in_anglvel_sampling_frequency";

        char* const accel_scale = read_file(iio->path, scale_main_file);
        if (accel_scale != NULL) {
            iio->anglvel_sampling_rate_hz = strtod(accel_scale, NULL);
            free((void*)accel_scale);

            if (inline_write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->anglvel_sampling_rate_hz = PREFERRED_SAMPLING_FREQ;
                printf("anglvel sampling rate changed to %f for device %s\n", iio->accel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_anglvel_sampling_frequency for device %s.\n", iio->name);
            }
        } else {
            fprintf(stderr, "Unable to read in_anglvel_sampling_frequency file from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_old_iio_create_err;
        }
    }
    // ==========================================================================================================

	// ======================================= accel_sampling_rate_hz ==========================================
    {
        const char* preferred_scale = PREFERRED_SAMPLING_FREQ_STR;
        const char *scale_main_file = "/in_accel_sampling_frequency";

        char* const accel_scale = read_file(iio->path, scale_main_file);
        if (accel_scale != NULL) {
            iio->accel_sampling_rate_hz = strtod(accel_scale, NULL);
            free((void*)accel_scale);

            if (inline_write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->accel_sampling_rate_hz = PREFERRED_SAMPLING_FREQ;
                printf("accel sampling rate changed to %f for device %s\n", iio->accel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_accel_sampling_frequency for device %s.\n", iio->name);
            }
        } else {
            fprintf(stderr, "Unable to read in_accel_sampling_frequency file from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_old_iio_create_err;
        }
    }
    // ==========================================================================================================

    const size_t tmp_sz = path_len + 128 + 1;
    char* const tmp = malloc(tmp_sz);

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_x_raw");
    iio->accel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_y_raw");
    iio->accel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_z_raw");
    iio->accel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_x_raw");
    iio->anglvel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_y_raw");
    iio->anglvel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_z_raw");
    iio->anglvel_z_fd = fopen(tmp, "r");

    free(tmp);

    printf(
        "anglvel scale: x=%f, y=%f, z=%f | accel scale: x=%f, y=%f, z=%f\n",
        iio->anglvel_scale_x,
        iio->anglvel_scale_y,
        iio->anglvel_scale_z,
        iio->accel_scale_x,
        iio->accel_scale_y,
        iio->accel_scale_z
    );

    // give time to change the scale
    sleep(4);

dev_old_iio_create_err:
    return iio;
}

static void dev_old_iio_destroy(dev_old_iio_t* iio) {
    fclose(iio->accel_x_fd);
    fclose(iio->accel_y_fd);
    fclose(iio->accel_z_fd);
    fclose(iio->anglvel_x_fd);
    fclose(iio->anglvel_y_fd);
    fclose(iio->anglvel_z_fd);
    free(iio->name);
    free(iio->path);
    free(iio);
}

const char* dev_old_iio_get_name(const dev_old_iio_t* iio) {
    return iio->name;
}

const char* dev_old_iio_get_path(const dev_old_iio_t* iio) {
    return iio->path;
}

static void multiplyMatrixVector(const double matrix[3][3], const double vector[3], double result[3]) {
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2];
}

int dev_old_iio_read_imu(const dev_old_iio_t *const iio, in_message_t *const messages) {
	int res = 0;

	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) == -1) {
        perror("Error getting time");
        return res; // Handle the error appropriately in your application
    }

	const uint64_t nanoseconds = (tp.tv_sec * 1000000000ULL) + tp.tv_nsec;

	uint16_t accel_x_raw = 0, accel_y_raw = 0, accel_z_raw = 0;
	uint16_t gyro_x_raw = 0, gyro_y_raw = 0, gyro_z_raw = 0;

    char tmp[128];


    if (iio->accel_x_fd != NULL) {
        rewind(iio->accel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_x_fd);
        if (tmp_read >= 0) {
            accel_x_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading accel(x): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

    if (iio->accel_y_fd != NULL) {
        rewind(iio->accel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_y_fd);
        if (tmp_read >= 0) {
            accel_y_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading accel(y): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

    if (iio->accel_z_fd != NULL) {
        rewind(iio->accel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_z_fd);
        if (tmp_read >= 0) {
            accel_z_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading accel(z): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

    if (iio->anglvel_x_fd != NULL) {
        rewind(iio->anglvel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_x_fd);
        if (tmp_read >= 0) {
            gyro_x_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading anglvel(x): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

    if (iio->anglvel_y_fd != NULL) {
        rewind(iio->anglvel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_y_fd);
        if (tmp_read >= 0) {
            gyro_y_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading anglvel(y): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

    if (iio->anglvel_z_fd != NULL) {
        rewind(iio->anglvel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_z_fd);
        if (tmp_read >= 0) {
            gyro_z_raw = strtol(&tmp[0], NULL, 10);
        } else {
            fprintf(stderr, "While reading anglvel(z): %d\n", tmp_read);
            goto dev_old_iio_read_imu_err;
        }
    }

	messages[0].type = GAMEPAD_SET_ELEMENT;
    messages[0].data.gamepad_set.element = GAMEPAD_ACCELEROMETER;
    messages[1].data.gamepad_set.status.accel.sample_timestamp_ns = nanoseconds;
    messages[0].data.gamepad_set.status.accel.x = (uint16_t)(-1) * accel_x_raw;
    messages[0].data.gamepad_set.status.accel.y = accel_y_raw;
    messages[0].data.gamepad_set.status.accel.z = accel_z_raw;

    messages[1].type = GAMEPAD_SET_ELEMENT;
    messages[1].data.gamepad_set.element = GAMEPAD_GYROSCOPE;
    messages[1].data.gamepad_set.status.gyro.sample_timestamp_ns = nanoseconds;
    messages[1].data.gamepad_set.status.gyro.x = (uint16_t)(-1) * gyro_x_raw;
    messages[1].data.gamepad_set.status.gyro.y = gyro_y_raw;
    messages[1].data.gamepad_set.status.gyro.z = gyro_z_raw;

	res = 2;

dev_old_iio_read_imu_err:
	return res;
}

typedef struct bmc150_accel_user_data {
	dev_old_iio_t *iio;
	char* name;
	uint64_t errors;
} bmc150_accel_user_data_t;

static bmc150_accel_user_data_t bmc150_timer_data = {
	.iio = NULL,
	.name = NULL,
	.errors = 0,
};

int rc71l_bmc150_accel_timer_map(const dev_in_settings_t *const conf, int timer_fd, uint64_t expirations, in_message_t *const messages, size_t messages_len, void* user_data) {
	bmc150_accel_user_data_t *const timer_data = (bmc150_accel_user_data_t*)user_data;

	static const uint64_t max_attempts = 250000000;

	if (timer_data == NULL) {
		return 0;
	}

	if (timer_data->iio == NULL) {
		if (timer_data->errors < max_attempts) {
			// try to open the device and give up after some errors
			timer_data->iio = dev_old_iio_create(iio_base_path);

			if (timer_data->iio == NULL) {
				timer_data->errors++;

				if (timer_data->errors == max_attempts) {
					fprintf(stderr, "Max attempts to acquire bmc150 accel driver reached.\n");
				}
			}
		}

		return 0;
	} else {
		// read the device and fill data from that
		return dev_old_iio_read_imu(timer_data->iio, messages);
	}

	return 0;
}

input_dev_t bmc150_timer_dev = {
	.dev_type = input_dev_type_timer,
	.filters = {
		.timer = {
			.name = "RC71L_bmc150-accel_timer",
			.ticktime_ms = 0,
			.ticktime_ns = 1250000
		}
	},
	.user_data = &bmc150_timer_data,
	.map = {
		.timer_callbacks = {
			.map_fn = rc71l_bmc150_accel_timer_map,
		}
	}
};

int rc71l_timer_map(const dev_in_settings_t *const conf, int timer_fd, uint64_t expirations, in_message_t *const messages, size_t messages_len, void* user_data) {
	rc71l_timer_user_data_t *const timer_data = (rc71l_timer_user_data_t*)user_data;
	rc71l_platform_t *const platform_data = timer_data->parent;

	if (platform_data == NULL) {
		return 0;
	}

	return 0;
}

input_dev_t timer_dev = {
	.dev_type = input_dev_type_timer,
	.filters = {
		.timer = {
			.name = "RC71L_timer",
			.ticktime_ms = 60,
			.ticktime_ns = 0,
		}
	},
	.user_data = &timer_user_data,
	.map = {
		.timer_callbacks = {
			.map_fn = rc71l_timer_map,
		}
	}
};

input_dev_composite_t rc71l_composite = {
  .dev = {
    &in_xbox_dev,
    &in_asus_kb_1_dev,
    &in_asus_kb_2_dev,
    &in_asus_kb_3_dev,
	&timer_dev,
  },
  .dev_count = 5,
  .init_fn = rc71l_platform_init,
  .deinit_fn = rc71l_platform_deinit,
  .leds_fn = rc71l_platform_leds,
};

input_dev_composite_t* rog_ally_device_def(const dev_in_settings_t *const conf) {
	if (conf->enable_imu) {
		bmc150_timer_data.name = read_file(iio_base_path, "name");
		if ((bmc150_timer_data.name != NULL) && (strcmp(bmc150_timer_data.name, "bmi323"))) {
			printf("Old bmc150-accel-i2c for bmi323 device has been selected! Are you running a neptune kernel?\n");
			rc71l_composite.dev[rc71l_composite.dev_count++] = &bmc150_timer_dev;
		} else if ((conf->imu_polling_interface)) {
			if (bmc150_timer_data.name != NULL) {
				printf("Forced polling on a %s, suspend/resume issues?\n", bmc150_timer_data.name);
				rc71l_composite.dev[rc71l_composite.dev_count++] = &bmc150_timer_dev;
			}
		} else {
			printf("Using the newer upstreamed bmi323-imu driver\n");
			rc71l_composite.dev[rc71l_composite.dev_count++] = &in_iio_dev;
		}
	}
	
	if (conf->touchbar) {
		rc71l_composite.dev[rc71l_composite.dev_count++] = &in_touchscreen_dev;
	}

	if ((conf->enable_leds_commands) || (conf->enable_thermal_profiles_switching)) {
		rc71l_composite.dev[rc71l_composite.dev_count++] = &nkey_dev;
	}

	if ((conf->enable_thermal_profiles_switching) && (conf->default_thermal_profile >= 0)) {
		hw_platform.current_thermal_profile = 0xFFFFFFFFFFFFFFFF;
		hw_platform.next_thermal_profile = conf->default_thermal_profile % PROFILES_COUNT;
	}

	return &rc71l_composite;
}
