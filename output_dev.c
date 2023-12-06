#include "output_dev.h"
#include "logic.h"
#include "platform.h"
#include "queue.h"
#include "message.h"
#include "settings.h"
#include "virt_ds4.h"
#include <pthread.h>

#define DECODE_EV_FLAG_MODE_SWITCH_REQUESTED	0x00000001U
#define DECODE_EV_FLAG_MODE_MAIN_MENU_REQUESTED 0x00000002U
#define DECODE_EV_FLAG_MODE_QAM_REQUESTED 		0x00000004U

static uint32_t decode_ev(output_dev_t *const out_dev, message_t *const msg) {
	uint32_t flags = 0x00000000U;

	// scan for mouse mode and emit events in the virtual mouse if required
	if ((is_rc71l_ready(out_dev->logic)) && (is_mouse_mode(&out_dev->logic->platform))) {
		// search for mouse-related events
		for (uint32_t a = 0; a < msg->data.event.ev_count; ++a) {
			if (msg->data.event.ev[a].type == EV_KEY) {
				if ((msg->data.event.ev[a].code == BTN_MIDDLE) || (msg->data.event.ev[a].code == BTN_LEFT) || (msg->data.event.ev[a].code == BTN_RIGHT)) {
					msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_PRESERVE_TIME | EV_MESSAGE_FLAGS_MOUSE;
					return flags;
				}
			} else if (msg->data.event.ev[a].type == EV_REL) {
				msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_PRESERVE_TIME | EV_MESSAGE_FLAGS_MOUSE;
				return flags;
			}
		}
	}

    if (msg->data.event.ev[0].type == EV_REL) {
        msg->data.event.ev_flags |= EV_MESSAGE_FLAGS_MOUSE;
    } else if ((msg->data.event.ev_count >= 2) && (msg->data.event.ev[0].type == EV_MSC) && (msg->data.event.ev[0].code == MSC_SCAN)) {
        if ((msg->data.event.ev[0].value == -13565784) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F18)) {
            if (msg->data.event.ev[1].value == 1) {
                flags |= DECODE_EV_FLAG_MODE_SWITCH_REQUESTED;
            } else {
                // Do nothing effectively discarding the input
            }

            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if (msg->data.event.ev[0].value == -13565784) {
            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if (
			(msg->data.event.ev_count == 2) &&
			(msg->data.event.ev[0].value == 458860) &&
			(msg->data.event.ev[1].type == EV_KEY) &&
			(msg->data.event.ev[1].code == KEY_F17)
		) {
            // this is the left back paddle while in game mode or mouse mode
        } else if (
			(msg->data.event.ev_count == 2) &&
			(msg->data.event.ev[0].value == 458861) &&
			(msg->data.event.ev[1].type == EV_KEY) &&
			(msg->data.event.ev[1].code == KEY_F18)
		) {
            // this is the left back paddle while in game mode or mouse mode
        } else if ((msg->data.event.ev_count == 2) && (msg->data.event.ev[0].value == -13565786) && (msg->data.event.ev[1].type == EV_KEY) && (msg->data.event.ev[1].code == KEY_F16)) {
            printf("Converted AC short-press button to BTN_MODE\n");
			msg->data.event.ev_count = 1;
            msg->data.event.ev[0].type = EV_KEY;
            msg->data.event.ev[0].code = BTN_MODE;
            msg->data.event.ev[0].value = msg->data.event.ev[1].value;

			flags |= DECODE_EV_FLAG_MODE_MAIN_MENU_REQUESTED;
        } else if (
			(msg->data.event.ev_count == 2) &&
			(msg->data.event.ev[0].value == -13565787) &&
			(msg->data.event.ev[1].type == EV_KEY) &&
			(msg->data.event.ev[1].code == KEY_F15)
		) {
            // this is default mode (deprecated) M15 for both back buttons and one was pressed. good luck with that!

            msg->flags |= INPUT_FILTER_FLAGS_DO_NOT_EMIT;
        } else if (
			(msg->data.event.ev_count == 2) &&
			(msg->data.event.ev_size >= 3) &&
			(msg->data.event.ev[0].value == -13565896) &&
			(msg->data.event.ev[1].type == EV_KEY) &&
			(msg->data.event.ev[1].code == KEY_PROG1)
		) {
			flags |= DECODE_EV_FLAG_MODE_QAM_REQUESTED;
        }
    }

	return flags;
}

static void update_gs_from_ev(devices_status_t *const stats, message_t *const msg, controller_settings_t *const settings) {
	if ( // this is what happens at release of the left-screen button of the ROG Ally
		(msg->data.event.ev_count == 2) &&
		(msg->data.event.ev[0].type == EV_MSC) &&
		(msg->data.event.ev[0].code == MSC_SCAN) &&
		(msg->data.event.ev[0].value == -13565786) &&
		(msg->data.event.ev[1].type == EV_KEY) &&
		(msg->data.event.ev[1].code == KEY_F16) &&
		(msg->data.event.ev[1].value == 1)
	) {
#if defined(INCLUDE_OUTPUT_DEBUG)
		printf("RC71L CC button short-press detected\n");
#endif
		stats->gamepad.flags |= GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
	} else if ( // this is what happens at release of the right-screen button of the ROG Ally
		(msg->data.event.ev_count == 2) &&
		(msg->data.event.ev[0].type == EV_MSC) &&
		(msg->data.event.ev[0].code == MSC_SCAN) &&
		(msg->data.event.ev[0].value == -13565896) &&
		(msg->data.event.ev[1].type == EV_KEY) &&
		(msg->data.event.ev[1].code == KEY_PROG1) &&
		(msg->data.event.ev[1].value == 1)
	) {
#if defined(INCLUDE_OUTPUT_DEBUG)
		printf("RC71L AC button short-press detected\n");
#endif
		if (settings->enable_qam) {
			stats->gamepad.flags |= GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
		}
	} else if (
		(msg->data.event.ev_count == 2) &&
		(msg->data.event.ev[0].type == EV_MSC) &&
		(msg->data.event.ev[0].code == MSC_SCAN) &&
		(msg->data.event.ev[0].value == 458860) &&
		(msg->data.event.ev[1].type == EV_KEY) &&
		(msg->data.event.ev[1].code == KEY_F17)
	) {
		stats->gamepad.l4 = msg->data.event.ev[1].value;
    } else if (
		(msg->data.event.ev_count == 2) &&
		(msg->data.event.ev[0].type == EV_MSC) &&
		(msg->data.event.ev[0].code == MSC_SCAN) &&
		(msg->data.event.ev[0].value == 458861) &&
		(msg->data.event.ev[1].type == EV_KEY) &&
		(msg->data.event.ev[1].code == KEY_F18)
	) {
		stats->gamepad.r4 = msg->data.event.ev[1].value;
	}

	for (uint32_t i = 0; i < msg->data.event.ev_count; ++i) {
		if (msg->data.event.ev[i].type == EV_KEY) {
			if (msg->data.event.ev[i].code == BTN_EAST) {
				if (settings->nintendo_layout) {
					stats->gamepad.cross = msg->data.event.ev[i].value;
				} else {
					stats->gamepad.circle = msg->data.event.ev[i].value;
				}
			} else if (msg->data.event.ev[i].code == BTN_NORTH) {
				if (settings->nintendo_layout) {
					stats->gamepad.triangle = msg->data.event.ev[i].value;
				} else {
					stats->gamepad.square = msg->data.event.ev[i].value;
				}
			} else if (msg->data.event.ev[i].code == BTN_SOUTH) {
				if (settings->nintendo_layout) {
					stats->gamepad.circle = msg->data.event.ev[i].value;
				} else {
					stats->gamepad.cross = msg->data.event.ev[i].value;
				}
			} else if (msg->data.event.ev[i].code == BTN_WEST) {
				if (settings->nintendo_layout) {
					stats->gamepad.square = msg->data.event.ev[i].value;
				} else {
					stats->gamepad.triangle = msg->data.event.ev[i].value;
				}
			} else if (msg->data.event.ev[i].code == BTN_SELECT) {
				stats->gamepad.option = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_START) {
				stats->gamepad.share = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_TR) {
				stats->gamepad.r1 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_TL) {
				stats->gamepad.l1 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_THUMBR) {
				stats->gamepad.r3 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_THUMBL) {
				stats->gamepad.l3 = msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == BTN_MODE) {
				stats->gamepad.flags |= GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
			}
		} else if (msg->data.event.ev[i].type == EV_ABS) {
			if (msg->data.event.ev[i].code == ABS_X) {
				stats->gamepad.joystick_positions[0][0] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_Y) {
				stats->gamepad.joystick_positions[0][1] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RX) {
				stats->gamepad.joystick_positions[1][0] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RY) {
				stats->gamepad.joystick_positions[1][1] = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_Z) {
				stats->gamepad.l2_trigger = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_RZ) {
				stats->gamepad.r2_trigger = (int32_t)msg->data.event.ev[i].value;
			} else if (msg->data.event.ev[i].code == ABS_HAT0X) {
				const int v = msg->data.event.ev[i].value;
				stats->gamepad.dpad &= 0xF0;
				if (v == 0) {
					stats->gamepad.dpad |= 0x00;
				} else if (v == 1) {
					stats->gamepad.dpad |= 0x01;
				} else if (v == -1) {
					stats->gamepad.dpad |= 0x02;
				}
			} else if (msg->data.event.ev[i].code == ABS_HAT0Y) {
				const int v = msg->data.event.ev[i].value;
				stats->gamepad.dpad &= 0x0F;
				if (v == 0) {
					stats->gamepad.dpad |= 0x00;
				} else if (v == 1) {
					stats->gamepad.dpad |= 0x20;
				} else if (v == -1) {
					stats->gamepad.dpad |= 0x10;
				}
			}
		}
	}
}

static void update_gs_from_imu(devices_status_t *const stats, message_t *const msg, controller_settings_t *const settings) {
	if (msg->data.imu.flags & IMU_MESSAGE_FLAGS_ANGLVEL) {
		stats->gamepad.last_gyro_motion_time = msg->data.imu.gyro_read_time;

		memcpy(stats->gamepad.gyro, msg->data.imu.gyro_rad_s, sizeof(double[3]));

		stats->gamepad.raw_gyro[0] = msg->data.imu.gyro_x_raw;
		stats->gamepad.raw_gyro[1] = msg->data.imu.gyro_y_raw;
		stats->gamepad.raw_gyro[2] = msg->data.imu.gyro_z_raw;
	}
	
	if (msg->data.imu.flags & IMU_MESSAGE_FLAGS_ACCEL) {
		stats->gamepad.last_accel_motion_time = msg->data.imu.accel_read_time;

		memcpy(stats->gamepad.accel, msg->data.imu.accel_m2s, sizeof(double[3]));

		stats->gamepad.raw_accel[0] = msg->data.imu.accel_x_raw;
		stats->gamepad.raw_accel[1] = msg->data.imu.accel_y_raw;
		stats->gamepad.raw_accel[2] = msg->data.imu.accel_z_raw;
	}
}

static void handle_msg(output_dev_t *const out_dev, message_t *const msg) {
	if (msg->type == MSG_TYPE_EV) {
		const uint32_t decode_ev_res_flags = decode_ev(out_dev, msg);

		if (decode_ev_res_flags & DECODE_EV_FLAG_MODE_SWITCH_REQUESTED) {
			printf("Detected mode switch command, switching mode...\n");

			// lock the mutex to flag every device as disconnected
			pthread_mutex_lock(&out_dev->logic->dev_stats.mutex);
			out_dev->logic->dev_stats.gamepad.connected = false;
			out_dev->logic->dev_stats.kbd.connected = false;
			pthread_mutex_unlock(&out_dev->logic->dev_stats.mutex);

			// wait for the thread to terminate itself...
			logic_terminate_output_thread(out_dev->logic);

			// re-lock the mutex and flag the controller as connected so that the new thread won't exit immediatly
			// as the other gamepad-accessing thread is now joined (has terminated) it's safe to update the state without locking the mutex
			out_dev->logic->dev_stats.gamepad.connected = true;

			const int cycle_mode_res = cycle_mode(&out_dev->logic->platform);
			if (cycle_mode_res == 0) {
				const int output_thread_start_res = (is_mouse_mode(&out_dev->logic->platform)) ?
					logic_start_output_mouse_kbd_thread(out_dev->logic) :
					logic_start_output_dev_thread(out_dev->logic);
				
				if (output_thread_start_res != 0) {
					fprintf(stderr, "Unable to start output thread: %d -- don't panic, another mode switch will retry.", output_thread_start_res);
				}
			} else {
				fprintf(stderr, "Error in mode switching: %d\n", cycle_mode_res);
			}
		}
	}

	const int upd_beg_res = pthread_mutex_lock(&out_dev->logic->dev_stats.mutex);
	if (upd_beg_res != 0) {
		fprintf(stderr, "Unable to begin the gamepad status update (can't lock mutex): %d\n", upd_beg_res);
		return;
	}

	if (msg->type == MSG_TYPE_EV) {
		update_gs_from_ev(&out_dev->logic->dev_stats, msg, &out_dev->logic->controller_settings);
	} else if (msg->type == MSG_TYPE_IMU) {
		update_gs_from_imu(&out_dev->logic->dev_stats, msg, &out_dev->logic->controller_settings);
	}

	pthread_mutex_unlock(&out_dev->logic->dev_stats.mutex);
}

/*
void *output_dev_rumble_thread_func(void* ptr) {
	output_dev_t *const out_dev = (output_dev_t*)ptr;

	struct timeval now = {0};

#if defined(INCLUDE_TIMESTAMP)
	gettimeofday(&now, NULL);
	__time_t secAtInit = now.tv_sec;
	__time_t usecAtInit = now.tv_usec;
#endif

	pthread_mutex_lock(&out_dev->logic->gamepad_mutex);
	uint64_t rumble_events_count = out_dev->logic->gamepad.rumble_events_count;
	pthread_mutex_unlock(&out_dev->logic->gamepad_mutex);

	// maximum number of ms that the gamepad can remain in a blocked status
	const int timeout_ms = 40;

    for (;;) {
		// sleep for about 16ms: this is an aggressive polling for rumble
		usleep(16000);

		// here transmit the rumble request to the input-device-handling components
		pthread_mutex_lock(&out_dev->logic->gamepad_mutex);
		uint64_t tmp_ev_count = out_dev->logic->gamepad.rumble_events_count;
		uint8_t right_motor = out_dev->logic->gamepad.motors_intensity[0];
		uint8_t left_motor = out_dev->logic->gamepad.motors_intensity[1];
		pthread_mutex_unlock(&out_dev->logic->gamepad_mutex);

		// check if the gamepad has notified the presence of a rumble event
		if (tmp_ev_count != rumble_events_count) {
			rumble_message_t *const rumble_msg = malloc(sizeof(rumble_message_t));
			if(rumble_msg != NULL) {
				rumble_msg->strong_magnitude = (uint16_t)left_motor << (uint16_t)8;
				rumble_msg->weak_magnitude = (uint16_t)right_motor << (uint16_t)8;

				const int rumble_emit_res = queue_push_timeout(&out_dev->logic->rumble_events_queue, (void*)rumble_msg, timeout_ms);
				if (rumble_emit_res == 0) {
#if defined(INCLUDE_OUTPUT_DEBUG)
					printf("Rumble request propagated\n");
#endif

					// update the rumble events counter: this rumble event was handled
					rumble_events_count = tmp_ev_count;
				} else {
#if defined(INCLUDE_OUTPUT_DEBUG)
					fprintf(stderr, "Error propating the rumble event: %d\n", rumble_emit_res);
#endif

					free(rumble_msg);
				}
			} else {
				fprintf(stderr, "Error allocating resources to request a rumble\n");
			}
		}
		
		if (logic_termination_requested(out_dev->logic)) {
            break;
        }
    }

    return NULL;
}
*/

void *output_dev_thread_func(void *ptr) {
	output_dev_t *const out_dev = (output_dev_t*)ptr;

#if defined(INCLUDE_TIMESTAMP)
	struct timeval now = {0};
	gettimeofday(&now, NULL);
	__time_t secAtInit = now.tv_sec;
	__time_t usecAtInit = now.tv_usec;
#endif

/*
	pthread_t rumble_thread;
	const int rumble_thread_creation = pthread_create(&rumble_thread, NULL, output_dev_rumble_thread_func, ptr);
	if (rumble_thread_creation != 0) {
		fprintf(stderr, "Error creating the rumble thread: %d -- rumble will not work.\n", rumble_thread_creation);
	}
*/

	pthread_mutex_lock(&out_dev->logic->dev_stats.mutex);
	uint64_t rumble_events_count = out_dev->logic->dev_stats.gamepad.rumble_events_count;
	pthread_mutex_unlock(&out_dev->logic->dev_stats.mutex);

    for (;;) {
		void *raw_ev;
		const int pop_res = queue_pop_timeout(&out_dev->logic->input_queue, &raw_ev, 1);
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

		if (logic_termination_requested(out_dev->logic)) {
            break;
        }

		// here transmit the rumble request to the input-device-handling components
		pthread_mutex_lock(&out_dev->logic->dev_stats.mutex);
		uint64_t tmp_ev_count = out_dev->logic->dev_stats.gamepad.rumble_events_count;
		uint8_t right_motor = out_dev->logic->dev_stats.gamepad.motors_intensity[0];
		uint8_t left_motor = out_dev->logic->dev_stats.gamepad.motors_intensity[1];
		pthread_mutex_unlock(&out_dev->logic->dev_stats.mutex);

		// check if the gamepad has notified the presence of a rumble event
		if (tmp_ev_count != rumble_events_count) {
			rumble_message_t *const rumble_msg = malloc(sizeof(rumble_message_t));
			if(rumble_msg != NULL) {
				rumble_msg->strong_magnitude = (uint16_t)left_motor << (uint16_t)8;
				rumble_msg->weak_magnitude = (uint16_t)right_motor << (uint16_t)8;

				const int rumble_emit_res = queue_push_timeout(&out_dev->logic->rumble_events_queue, (void*)rumble_msg, 0);
				if (rumble_emit_res == 0) {
#if defined(INCLUDE_OUTPUT_DEBUG)
					printf("Rumble request propagated\n");
#endif

					// update the rumble events counter: this rumble event was handled
					rumble_events_count = tmp_ev_count;
				} else {
#if defined(INCLUDE_OUTPUT_DEBUG)
					fprintf(stderr, "Error propating the rumble event: %d\n", rumble_emit_res);
#endif

					free(rumble_msg);
				}
			} else {
				fprintf(stderr, "Error allocating resources to request a rumble\n");
			}
		}
    }
/*
	pthread_join(rumble_thread, NULL);
*/
    return NULL;
}
