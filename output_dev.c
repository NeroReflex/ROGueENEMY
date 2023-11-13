#include "output_dev.h"
#include "queue.h"
#include "message.h"
#include <linux/input-event-codes.h>
#include <unistd.h>

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

void *output_dev_thread_func(void *ptr) {
    output_dev_t *out_dev = (output_dev_t*)ptr;
	struct timeval now = {0};

#if defined(INCLUDE_TIMESTAMP)
	gettimeofday(&now, NULL);
	__time_t secAtInit = now.tv_sec;
	__time_t usecAtInit = now.tv_usec;
#endif

    for (;;) {
		void *raw_ev;
		const int pop_res = queue_pop_timeout(out_dev->queue, &raw_ev, 1000);
		if (pop_res == 0) {
			message_t *const msg = (message_t*)raw_ev;

			int fd = out_dev->gamepad_fd;
			if ((msg->flags & INPUT_FILTER_FLAGS_IMU) != 0) {
				fd = out_dev->imu_fd;
			} else if ((msg->flags & INPUT_FILTER_FLAGS_MOUSE) != 0) {
				fd = out_dev->mouse_fd;
			} else {
				fd = out_dev->gamepad_fd;
			}

			for (uint32_t i = 0; i < msg->ev_count; ++i) {
				struct input_event ev = {
					.code = msg->ev[i].code,
					.type = msg->ev[i].type,
					.value = msg->ev[i].value,
					.time = msg->ev[i].time,
				};

				if ((msg->flags & INPUT_FILTER_FLAGS_PRESERVE_TIME) == 0) {
					gettimeofday(&now, NULL);
					ev.time = now;
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

				if (fd == out_dev->mouse_fd) {
					printf(
						"type: %s, code: %s, value: %d\n",
						libevdev_event_type_get_name(ev.type),
						libevdev_event_code_get_name(ev.type, ev.code),
						(int)ev.value	
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
