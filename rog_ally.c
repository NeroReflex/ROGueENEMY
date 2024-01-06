#include "rog_ally.h"
#include "input_dev.h"
#include "dev_hidraw.h"
#include "message.h"
#include "xbox360.h"

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

typedef struct rc71l_timer_user_data {
	struct rc71l_platform* parent;

} rc71l_timer_user_data_t;

rc71l_timer_user_data_t timer_user_data;

typedef struct rc71l_platform {
  rc71l_asus_kbd_user_data_t* kbd_user_data;

  rc71l_xbox360_user_data_t* xbox360_user_data;

  rc71l_timer_user_data_t* timer_data;

  struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
  } static_led_color;

} rc71l_platform_t;

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

static int rc71l_platform_init(const dev_in_settings_t *const conf, void** platform_data) {
	int res = -EINVAL;

	rc71l_platform_t *const platform = &hw_platform;
	*platform_data = (void*)platform;

	// setup asus keyboard(s) user_data
	platform->kbd_user_data->parent = platform;
	platform->xbox360_user_data->parent = platform;
	platform->timer_data->parent = platform;
	platform->kbd_user_data->udev = udev_new();
	if (platform->kbd_user_data->udev == NULL) {
		fprintf(stderr, "Unable to initialize udev\n");
		res = -ENOMEM;
		goto rc71l_platform_init_err;
	}

	res = 0;

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

	if (platform_data == NULL) {
		return 0;
	}

	platform->static_led_color.r = r;
	platform->static_led_color.g = g;
	platform->static_led_color.b = b;

	char command_str[64] = "\0";
	sprintf(command_str, "asusctl led-mode static -c %02X%02X%02X", r ,g ,b);
	
	return system(command_str);
}

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
    &in_iio_dev,
    &in_asus_kb_1_dev,
    &in_asus_kb_2_dev,
    &in_asus_kb_3_dev,
	&timer_dev,
	&in_touchscreen_dev,
  },
  .dev_count = 7,
  .init_fn = rc71l_platform_init,
  .deinit_fn = rc71l_platform_deinit,
  .leds_fn = rc71l_platform_leds,
};

input_dev_composite_t* rog_ally_device_def(const dev_in_settings_t *const settings) {
	if (!settings->touchbar) {
		// this is because the touchscreen is the latest in the list
		rc71l_composite.dev_count -= 1;
	}

	return &rc71l_composite;
}
