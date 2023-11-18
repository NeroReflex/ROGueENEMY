#include "output_dev.h"
#include "logic.h"
#include "queue.h"
#include "message.h"
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <unistd.h>

#include "virt_ds4.h"

int create_output_dev(const char* uinput_path, output_dev_type_t type) {
    int fd = open(uinput_path, O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
        fd = -1;
        close(fd);
        goto create_output_dev_err;
    }
	
	struct uinput_setup dev = {0};
	strncpy(dev.name, OUTPUT_DEV_NAME, UINPUT_MAX_NAME_SIZE-1);

#if defined(OUTPUT_DEV_BUS_TYPE)
	dev.id.bustype = OUTPUT_DEV_BUS_TYPE;
#else
	dev.id.bustype = BUS_VIRTUAL;
#endif
	dev.id.vendor = OUTPUT_DEV_VENDOR_ID;
	dev.id.product = OUTPUT_DEV_PRODUCT_ID;

#if defined(OUTPUT_DEV_VERSION)
	dev.id.version = OUTPUT_DEV_VERSION;
#endif

	//if (ioctl(fd, /*UI_SET_PHYS_STR*/ 18, PHYS_STR) != 0) {
	//	fprintf(stderr, "Controller and gyroscope will NOT be recognized as a single device!\n");
	//}

	if (ioctl(fd, UI_SET_PHYS, PHYS_STR) != 0) {
		fprintf(stderr, "Error setting the phys of the virtual controller.\n");
	}

#if !defined(UI_SET_PHYS_STR)
	#warning Will not change the PHYS
#endif

#if !defined(UI_SET_UNIQ_STR)
	#warning Will not change the UNIQ
#endif

	switch (type) {
		case output_dev_imu: {
#if defined(UI_SET_PHYS_STR)
			ioctl(fd, UI_SET_PHYS_STR(18), PHYS_STR);
#else
			fprintf(stderr, "UI_SET_PHYS_STR unavailable.\n");
#endif

#if defined(UI_SET_UNIQ_STR)
			ioctl(fd, UI_SET_UNIQ_STR(18), PHYS_STR);
#else
			fprintf(stderr, "UI_SET_UNIQ_STR unavailable.\n");
#endif

			ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_ACCELEROMETER);
			ioctl(fd, UI_SET_EVBIT, EV_ABS);
#if defined(INCLUDE_TIMESTAMP)
			ioctl(fd, UI_SET_EVBIT, EV_MSC);
			ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
#endif

			ioctl(fd, UI_SET_ABSBIT, ABS_X);
			ioctl(fd, UI_SET_ABSBIT, ABS_Y);
			ioctl(fd, UI_SET_ABSBIT, ABS_Z);
			ioctl(fd, UI_SET_ABSBIT, ABS_RX);
			ioctl(fd, UI_SET_ABSBIT, ABS_RY);
			ioctl(fd, UI_SET_ABSBIT, ABS_RZ);

			//ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER);
			//ioctl(fd, UI_SET_KEYBIT, BTN_THUMB);
			
			struct uinput_abs_setup devAbsX = {0};
			devAbsX.code = ABS_X;
			devAbsX.absinfo.minimum = -ACCEL_RANGE;
			devAbsX.absinfo.maximum = ACCEL_RANGE;
			devAbsX.absinfo.resolution = 255; // 255 units = 1g
			devAbsX.absinfo.fuzz = 5;
			devAbsX.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsY = {0};
			devAbsY.code = ABS_Y;
			devAbsY.absinfo.minimum = -ACCEL_RANGE;
			devAbsY.absinfo.maximum = ACCEL_RANGE;
			devAbsY.absinfo.resolution = 255; // 255 units = 1g
			devAbsY.absinfo.fuzz = 5;
			devAbsY.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsZ = {0};
			devAbsZ.code = ABS_Z;
			devAbsZ.absinfo.minimum = -ACCEL_RANGE;
			devAbsZ.absinfo.maximum = ACCEL_RANGE;
			devAbsZ.absinfo.resolution = 255; // 255 units = 1g
			devAbsZ.absinfo.fuzz = 5;
			devAbsZ.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRX = {0};
			devAbsRX.code = ABS_RX;
			devAbsRX.absinfo.minimum = -GYRO_RANGE;
			devAbsRX.absinfo.maximum = GYRO_RANGE;
			devAbsRX.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRX.absinfo.fuzz = 0;
			devAbsRX.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsRY = {0};
			devAbsRY.code = ABS_RY;
			devAbsRY.absinfo.minimum = -GYRO_RANGE;
			devAbsRY.absinfo.maximum = GYRO_RANGE;
			devAbsRY.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRY.absinfo.fuzz = 0;
			devAbsRY.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRZ = {0};
			devAbsRZ.code = ABS_RZ;
			devAbsRZ.absinfo.minimum = -GYRO_RANGE;
			devAbsRZ.absinfo.maximum = GYRO_RANGE;
			devAbsRZ.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRZ.absinfo.fuzz = 0;
			devAbsRZ.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
				
			if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			if(ioctl(fd, UI_DEV_CREATE) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			break;
		}

		case output_dev_gamepad: {
#if defined(UI_SET_PHYS_STR)
			ioctl(fd, UI_SET_PHYS_STR(18), PHYS_STR);
#else
			fprintf(stderr, "UI_SET_PHYS_STR unavailable.\n");
#endif

#if defined(UI_SET_UNIQ_STR)
			ioctl(fd, UI_SET_UNIQ_STR(18), PHYS_STR);
#else
			fprintf(stderr, "UI_SET_UNIQ_STR unavailable.\n");
#endif

			//ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_BUTTONPAD);
			ioctl(fd, UI_SET_EVBIT, EV_ABS);
			ioctl(fd, UI_SET_EVBIT, EV_KEY);
			ioctl(fd, UI_SET_EVBIT, EV_SYN);
#if defined(INCLUDE_TIMESTAMP)
			ioctl(fd, UI_SET_EVBIT, EV_MSC);
			ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
#endif

			ioctl(fd, UI_SET_ABSBIT, ABS_X);
			ioctl(fd, UI_SET_ABSBIT, ABS_Y);
			ioctl(fd, UI_SET_ABSBIT, ABS_Z);
			ioctl(fd, UI_SET_ABSBIT, ABS_RX);
			ioctl(fd, UI_SET_ABSBIT, ABS_RY);
			ioctl(fd, UI_SET_ABSBIT, ABS_RZ);
			ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);
			ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);
			ioctl(fd, UI_SET_ABSBIT, ABS_HAT2X);
			ioctl(fd, UI_SET_ABSBIT, ABS_HAT2Y);

			ioctl(fd, UI_SET_KEYBIT, BTN_SOUTH);
			ioctl(fd, UI_SET_KEYBIT, BTN_EAST);
			ioctl(fd, UI_SET_KEYBIT, BTN_NORTH);
			ioctl(fd, UI_SET_KEYBIT, BTN_WEST);
			ioctl(fd, UI_SET_KEYBIT, BTN_TL);
			ioctl(fd, UI_SET_KEYBIT, BTN_TR);
			ioctl(fd, UI_SET_KEYBIT, BTN_TL2);
			ioctl(fd, UI_SET_KEYBIT, BTN_TR2);
			ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
			ioctl(fd, UI_SET_KEYBIT, BTN_START);
			ioctl(fd, UI_SET_KEYBIT, BTN_MODE);
			ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);
			ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);
			ioctl(fd, UI_SET_KEYBIT, BTN_GEAR_DOWN);
			ioctl(fd, UI_SET_KEYBIT, BTN_GEAR_UP);
			ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
		    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
		    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
		    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

			const struct uinput_abs_setup devAbsX = {
				.code = ABS_X,
				.absinfo = {
					.value = 0,
					.minimum = -32768,
					.maximum = +32768,
					.resolution = 1,
					.fuzz = 16,
					.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsY = {
				.code = ABS_Y,
				.absinfo = {
					.value = 0,
					.minimum = -32768,
					.maximum = +32768,
					.resolution = 1,
					.fuzz = 16,
					.flat = 128,
				}
			};

			if(ioctl(fd, UI_ABS_SETUP, &devAbsY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsZ = {
				.code = ABS_Z,
				.absinfo = {
					.value = 0,
					.minimum = 0,
					.maximum = 255,
					.resolution = 1,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRX = {
				.code = ABS_RX,
				.absinfo = {
					.value = 0,
					.minimum = -32768,
					.maximum = +32768,
					.resolution = 1,
					.fuzz = 16,
					.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsRY = {
				.code = ABS_RY,
				.absinfo = {
					.value = -1,
					.minimum = -32768,
					.maximum = +32768,
					.resolution = 1,
					.fuzz = 16,
					.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRZ = {
				.code = ABS_RZ,
				.absinfo = {
					.value = 0,
					.minimum = 0,
					.maximum = 255,
					.resolution = 1,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsHat0X = {
				.code = ABS_HAT0X,
				.absinfo = {
					.value = 0,
					.minimum = -1,
					.maximum = 1,
					.resolution = 1,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsHat0X) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsHat0Y = {
				.code = ABS_HAT0Y,
				.absinfo = {
					.value = 0,
					.minimum = -1,
					.maximum = 1,
					.resolution = 1,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsHat0Y) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsHat2X = {
				.code = ABS_HAT2X,
				.absinfo = {
					.value = 0,
					.minimum = 0,
					.maximum = 255,
					.resolution = 51,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsHat2X) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsHat2Y = {
				.code = ABS_HAT2Y,
				.absinfo = {
					.value = 0,
					.minimum = 0,
					.maximum = 255,
					.resolution = 51,
					//.fuzz = 16,
					//.flat = 128,
				}
			};
			if(ioctl(fd, UI_ABS_SETUP, &devAbsHat2Y) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			if(ioctl(fd, UI_DEV_CREATE) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			break;
		}

		case output_dev_mouse: {
			ioctl(fd, UI_SET_EVBIT, EV_REL);
			ioctl(fd, UI_SET_EVBIT, EV_KEY);
			ioctl(fd, UI_SET_EVBIT, EV_MSC);
			ioctl(fd, UI_SET_EVBIT, EV_SYN);
#if defined(INCLUDE_TIMESTAMP)
			ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
#endif

			ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
			ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
			ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
			//ioctl(fd, UI_SET_KEYBIT, BTN_SIDE);
			//ioctl(fd, UI_SET_KEYBIT, BTN_EXTRA);

			ioctl(fd, UI_SET_RELBIT, REL_X);
			ioctl(fd, UI_SET_RELBIT, REL_Y);
			ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
			ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);

			if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			if(ioctl(fd, UI_DEV_CREATE) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			break;
		}

		default:
			// error
			close(fd);
			fd = -1;
			goto create_output_dev_err;
	}
	
create_output_dev_err:
    return fd;
}

static void emit_ev(output_dev_t *const out_dev, const message_t *const msg) {
	// if events are flagged as do not emit... Do NOT emit!
	if (msg->flags & INPUT_FILTER_FLAGS_DO_NOT_EMIT) {
		return;
	}

	int fd = out_dev->gamepad_fd;
	if ((msg->flags & EV_MESSAGE_FLAGS_IMU) != 0) {
		fd = out_dev->imu_fd;
	} else if ((msg->flags & EV_MESSAGE_FLAGS_MOUSE) != 0) {
		fd = out_dev->mouse_fd;
	} else {
		fd = out_dev->gamepad_fd;
	}

	for (uint32_t i = 0; i < msg->data.event.ev_count; ++i) {
		struct input_event ev = {
			.code = msg->data.event.ev[i].code,
			.type = msg->data.event.ev[i].type,
			.value = msg->data.event.ev[i].value,
			.time = msg->data.event.ev[i].time,
		};

		if ((msg->data.event.ev_flags & EV_MESSAGE_FLAGS_PRESERVE_TIME) == 0) {
			gettimeofday(&ev.time, NULL);
		}

#if defined(INCLUDE_OUTPUT_DEBUG)
		printf(
			"Output: Received event %s (%s): %d\n",
			libevdev_event_type_get_name(ev.type),
			libevdev_event_code_get_name(ev.type, ev.code),
			ev.value
		);
#endif

		const ssize_t written = write(fd, (void*)&ev, sizeof(ev));
		if (written != sizeof(ev)) {
			fprintf(
				stderr,
				"Error writing event %s %s %d: written %ld bytes out of %ld\n",
				libevdev_event_type_get_name(ev.type),
				libevdev_event_code_get_name(ev.type, ev.code),
				ev.value,
				written,
				sizeof(ev)
			);
		}
	}

#if defined(INCLUDE_TIMESTAMP)
	gettimeofday(&now, NULL);
	const struct input_event timestamp_ev = {
		.code = MSC_TIMESTAMP,
		.type = EV_MSC,
		.value = (now.tv_sec - secAtInit)*1000000 + (now.tv_usec - usecAtInit),
		.time = now,
	};
	const ssize_t timestamp_written = write(fd, (void*)&timestamp_ev, sizeof(timestamp_ev));
	if (timestamp_written != sizeof(timestamp_ev)) {
		fprintf(stderr, "Error in sync: written %ld bytes out of %ld\n", timestamp_written, sizeof(timestamp_ev));
	}
#endif

	struct timeval now = {0};
	gettimeofday(&now, NULL);
	const struct input_event syn_ev = {
		.code = SYN_REPORT,
		.type = EV_SYN,
		.value = 0,
		.time = now,
	};
	const ssize_t sync_written = write(fd, (void*)&syn_ev, sizeof(syn_ev));
	if (sync_written != sizeof(syn_ev)) {
		fprintf(stderr, "Error in sync: written %ld bytes out of %ld\n", sync_written, sizeof(syn_ev));
	}
}

static void decode_ev(output_dev_t *const out_dev, message_t *const msg) {
	static int F15_status = 0;
	static int gyroscope_mouse_translation = 0;

    if (msg->data.event.ev[0].type == EV_REL) {
        msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_MOUSE;
    } else if ((msg->data.event.ev[0].type == EV_KEY) || (msg->data.event.ev[1].type == EV_KEY)) {
        if ((msg->data.event.ev[0].code == BTN_MIDDLE) || (msg->data.event.ev[0].code == BTN_LEFT) || (msg->data.event.ev[0].code == BTN_RIGHT)) {
            msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_PRESERVE_TIME | EV_MESSAGE_FLAGS_MOUSE;
        } else if ((msg->data.event.ev[1].code == BTN_MIDDLE) || (msg->data.event.ev[1].code == BTN_LEFT) || (msg->data.event.ev[1].code == BTN_RIGHT)) {
            msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_PRESERVE_TIME | EV_MESSAGE_FLAGS_MOUSE;
        }
    } else if ((msg->data.event.ev_count >= 2) && (msg->data.event.ev[0].type == EV_MSC) && (msg->data.event.ev[0].code == MSC_SCAN)) {
        if ((msg->data.event.ev[0].value == -13565784) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F18)) {
            if (msg->data.event.ev[1].value == 1) {
                printf("Detected mode switch command, switching mode...\n");
                cycle_mode(&out_dev->logic->platform);
            } else {
                // Do nothing effectively discarding the input
            }

            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if (msg->data.event.ev[0].value == -13565784) {
            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if ((msg->data.event.ev_count == 2) && (msg->data.event.ev[0].value == 458860) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F17)) {
            if (is_rc71l_ready(out_dev->logic)) {
				if (is_mouse_mode(&out_dev->logic->platform)) {
					if (msg->data.event.ev[1].value < 2) {
						msg->data.event.ev_count = 1;
						msg->data.event.ev[0].type = EV_KEY;
						msg->data.event.ev[0].code = BTN_GEAR_DOWN;
						msg->data.event.ev[0].value = msg->data.event.ev[1].value;
						return;
					}
				} else if (is_gamepad_mode(&out_dev->logic->platform)) {
					if (msg->data.event.ev[1].value == 0) {
						--gyroscope_mouse_translation;
					} else if (msg->data.event.ev[1].value == 1) {
						++gyroscope_mouse_translation;
					}
				}

				msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
			}
        } else if ((msg->data.event.ev_count == 2) && (msg->data.event.ev[0].value == 458861) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F18)) {
            if (is_rc71l_ready(out_dev->logic)) {
				if (is_mouse_mode(&out_dev->logic->platform)) {
					if (msg->data.event.ev[1].value < 2) {
						msg->data.event.ev_count = 1;
						msg->data.event.ev[0].type = EV_KEY;
						msg->data.event.ev[0].code = BTN_GEAR_UP;
						msg->data.event.ev[0].value = msg->data.event.ev[1].value;
					}
				} else if (is_gamepad_mode(&out_dev->logic->platform)) {
					if (msg->data.event.ev[1].value == 0) {
						--gyroscope_mouse_translation;
					} else if (msg->data.event.ev[1].value == 1) {
						++gyroscope_mouse_translation;
					}

					msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
				}
			}
        } else if ((msg->data.event.ev_count == 2) && (msg->data.event.ev[0].value == -13565786) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F16)) {
            msg->data.event.ev_count = 1;
            msg->data.event.ev[0].type = EV_KEY;
            msg->data.event.ev[0].code = BTN_MODE;
            msg->data.event.ev[0].value = msg->data.event.ev[1].value;
        } else if ((msg->data.event.ev_count == 2) && (msg->data.event.ev[0].value == -13565787) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F15)) {
            if (msg->data.event.ev[1].value == 0) {
                if (F15_status > 0) {
                    --F15_status;
                }

                if (F15_status == 0) {
                    printf("Exiting gyro mode.\n");
                }
            } else if (msg->data.event.ev[1].value == 1) {
                if (F15_status <= 2) {
                    ++F15_status;
                }

                if (F15_status == 1) {
                    printf("Entering gyro mode.\n");
                }
            }

            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if (
			(msg->data.event.ev_count == 2) &&
			(msg->data.event.ev_size >= 3) &&
			(msg->data.event.ev[0].value == -13565896) &&
			(msg->data.event.ev[1].type == EV_KEY) &&
			(msg->data.event.ev[1].code == KEY_PROG1)
		) {
            msg->data.event.ev_count = 3;

            const int32_t val = msg->data.event.ev[1].value;
            //struct timeval time = msg->data.event.ev[1].time;

            msg->data.event.ev[0].type = EV_KEY;
            msg->data.event.ev[0].code = BTN_MODE;
            msg->data.event.ev[0].value = val;

            msg->data.event.ev[1].type = SYN_REPORT;
            msg->data.event.ev[1].code = EV_SYN;
            msg->data.event.ev[1].value = 0;

            msg->data.event.ev[2].type = EV_KEY;
            msg->data.event.ev[2].code = BTN_SOUTH;
            msg->data.event.ev[2].value = val;
/*
            events[3].type = SYN_REPORT;
            events[3].code = EV_SYN;
            events[3].value = 0;

            events[4].type = EV_KEY;
            events[4].code = BTN_SOUTH;
            events[4].value = 0;
*/
        }
    }
}

static void update_gs_from_ev(gamepad_status_t *const gs, message_t *const msg) {
	for (uint32_t i = 0; i < msg->data.event.ev_count; ++i) {
		if (msg->data.event.ev[i].type == EV_KEY) {
			if (msg->data.event.ev[i].code == BTN_EAST) {
				gs->circle = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_NORTH) {
				gs->square = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_SOUTH) {
				gs->cross = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_WEST) {
				gs->triangle = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_SELECT) {
				gs->option = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_START) {
				gs->share = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_TR) {
				gs->r1 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_TL) {
				gs->l1 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_THUMBR) {
				gs->r3 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_THUMBL) {
				gs->l3 = msg->data.event.ev[i].value;
			}
		} else if (msg->data.event.ev[i].type == EV_ABS) {
			if (msg->data.event.ev[i].code == ABS_X) {
				gs->joystick_positions[0][0] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_Y) {
				gs->joystick_positions[0][1] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RX) {
				gs->joystick_positions[1][0] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RY) {
				gs->joystick_positions[1][1] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_Z) {
				gs->l2_trigger = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RZ) {
				gs->r2_trigger = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_HAT0X) {
				const int v = msg->data.event.ev[i].value;
				gs->dpad &= 0xF0;
				if (v == 0) {
					gs->dpad |= 0x00;
				} else if (v == 1) {
					gs->dpad |= 0x01;
				} else if (v == -1) {
					gs->dpad |= 0x02;
				}
			} else if (msg->data.event.ev[i].code == ABS_HAT0Y) {
				const int v = msg->data.event.ev[i].value;
				gs->dpad &= 0x0F;
				if (v == 0) {
					gs->dpad |= 0x00;
				} else if (v == 1) {
					gs->dpad |= 0x20;
				} else if (v == -1) {
					gs->dpad |= 0x10;
				}
			}
		}
	}
}

static void handle_msg(output_dev_t *const out_dev, message_t *const msg) {
	if (msg->type == MSG_TYPE_EV) {
		decode_ev(out_dev, msg);

		const int upd_beg_res = logic_begin_status_update(out_dev->logic);
		if (upd_beg_res == 0) {
			update_gs_from_ev(&out_dev->logic->gamepad, msg);

			logic_end_status_update(out_dev->logic);
		} else {
			fprintf(stderr, "[ev] Unable to begin the gamepad status update: %d\n", upd_beg_res);
		}

		if ((out_dev->logic->flags & LOGIC_FLAGS_VIRT_DS4_ENABLE) == 0) {
			emit_ev(out_dev, msg);
		}
	} else if (msg->type == MSG_TYPE_IMU) {
		const int upd_beg_res = logic_begin_status_update(out_dev->logic);
		if (upd_beg_res == 0) {
			out_dev->logic->gamepad.accel_x = msg->data.imu.accel_x_raw;
			out_dev->logic->gamepad.accel_y = msg->data.imu.accel_y_raw;
			out_dev->logic->gamepad.accel_z = msg->data.imu.accel_z_raw;
			out_dev->logic->gamepad.gyro_x = msg->data.imu.gyro_x_raw;
			out_dev->logic->gamepad.gyro_y = msg->data.imu.gyro_y_raw;
			out_dev->logic->gamepad.gyro_z = msg->data.imu.gyro_z_raw;

			logic_end_status_update(out_dev->logic);

#if defined(INCLUDE_OUTPUT_DEBUG)
			printf("gyro_x: %d\t\t| gyro_y: %d\t\t| gyro_z: %d\t\t\n", (int)msg->data.imu.gyro_x_raw, (int)msg->data.imu.gyro_y_raw, (int)msg->data.imu.gyro_z_raw);
#endif
		} else {
			fprintf(stderr, "[imu] Unable to begin the gamepad status update: %d\n", upd_beg_res);
		}
	}
}

void *output_dev_thread_func(void *ptr) {
	output_dev_t *const out_dev = (output_dev_t*)ptr;

	struct timeval now = {0};

#if defined(INCLUDE_TIMESTAMP)
	gettimeofday(&now, NULL);
	__time_t secAtInit = now.tv_sec;
	__time_t usecAtInit = now.tv_usec;
#endif

    for (;;) {
		void *raw_ev;
		const int pop_res = queue_pop_timeout(&out_dev->logic->input_queue, &raw_ev, 1000);
		if (pop_res == 0) {
			message_t *const msg = (message_t*)raw_ev;
			handle_msg(out_dev, msg);

			// from now on it's forbidden to use this memory
			msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
		} else if (pop_res == -1) {
			// timed out read
		} else {
			fprintf(stderr, "Cannot read from input queue: %d\n", pop_res);
			continue;
		}

        const uint32_t flags = out_dev->crtl_flags;
		if (flags & OUTPUT_DEV_CTRL_FLAG_EXIT) {
			out_dev->crtl_flags &= ~OUTPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }
    }

    return NULL;
}
