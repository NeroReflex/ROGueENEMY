#include "dev_out.h"

#include "devices_status.h"
#include "ipc.h"
#include "message.h"
#include "virt_ds4.h"
#include "virt_ds5.h"
#include "virt_mouse.h"
#include "virt_kbd.h"

#include <libconfig.h>

static void handle_incoming_message_gamepad_action(
    const dev_out_settings_t *const in_settings,
    const in_message_gamepad_action_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    if (*msg_payload == GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER) {
        inout_gamepad->flags |= GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
    } else if (*msg_payload == GAMEPAD_ACTION_OPEN_STEAM_QAM) {
        inout_gamepad->flags |= GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
    } 
}

static void handle_incoming_message_mouse_event(
    const dev_out_settings_t *const in_settings,
    const in_message_mouse_event_t *const msg_payload,
    mouse_status_t *const inout_mouse
) {
    if (msg_payload->type == MOUSE_ELEMENT_X) {
        inout_mouse->x += msg_payload->value;
    } else if (msg_payload->type == MOUSE_ELEMENT_Y) {
        inout_mouse->y += msg_payload->value;
    } else if (msg_payload->type == MOUSE_BTN_LEFT) {
        inout_mouse->btn_left = msg_payload->value;
    } else if (msg_payload->type == MOUSE_BTN_MIDDLE) {
        inout_mouse->btn_middle = msg_payload->value;
    } else if (msg_payload->type == MOUSE_BTN_RIGHT) {
        inout_mouse->btn_right = msg_payload->value;
    }
}

static void handle_incoming_message_gamepad_set(
    const dev_out_settings_t *const in_settings,
    const in_message_gamepad_set_element_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    static int numpackets = 0;
    static uint64_t lastsec = 0;
    switch (msg_payload->element) {
        case GAMEPAD_BTN_CROSS: {
            if (!in_settings->nintendo_layout) {
                inout_gamepad->cross = msg_payload->status.btn;
            } else {
                inout_gamepad->circle = msg_payload->status.btn;
            }
            
            break;
        }
        case GAMEPAD_BTN_CIRCLE: {
            if (in_settings->nintendo_layout) {
                inout_gamepad->cross = msg_payload->status.btn;
            } else {
                inout_gamepad->circle = msg_payload->status.btn;
            }

            break;
        }
        case GAMEPAD_BTN_SQUARE: {
            if (in_settings->nintendo_layout) {
                inout_gamepad->triangle = msg_payload->status.btn;
            } else {
                inout_gamepad->square = msg_payload->status.btn;
            }
            
            break;
        }
        case GAMEPAD_BTN_TRIANGLE: {
            if (!in_settings->nintendo_layout) {
                inout_gamepad->triangle = msg_payload->status.btn;
            } else {
                inout_gamepad->square = msg_payload->status.btn;
            }

            break;
        }
        case GAMEPAD_BTN_OPTION: {
            inout_gamepad->option = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_SHARE: {
            inout_gamepad->share = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_L1: {
            inout_gamepad->l1 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_R1: {
            inout_gamepad->r1 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_L2_TRIGGER: {
            inout_gamepad->l2_trigger = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_R2_TRIGGER: {
            inout_gamepad->r2_trigger = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_L3: {
            inout_gamepad->l3 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_R3: {
            inout_gamepad->r3 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_L4: {
            inout_gamepad->l4 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_R4: {
            inout_gamepad->r4 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_L5: {
            inout_gamepad->l5 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_R5: {
            inout_gamepad->r5 = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_TOUCHPAD: {
            inout_gamepad->touchpad_press = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_LEFT_JOYSTICK_X: {
            inout_gamepad->joystick_positions[0][0] = msg_payload->status.joystick_pos;
            break;
        }
        case GAMEPAD_LEFT_JOYSTICK_Y: {
            inout_gamepad->joystick_positions[0][1] = msg_payload->status.joystick_pos;
            break;
        }
        case GAMEPAD_RIGHT_JOYSTICK_X: {
            inout_gamepad->joystick_positions[1][0] = msg_payload->status.joystick_pos;
            break;
        }
        case GAMEPAD_RIGHT_JOYSTICK_Y: {
            inout_gamepad->joystick_positions[1][1] = msg_payload->status.joystick_pos;
            break;
        }
        case GAMEPAD_DPAD_X: {
            const int8_t v = msg_payload->status.dpad;

            inout_gamepad->dpad &= 0xF0;
            if (v == 0) {
                inout_gamepad->dpad |= 0x00;
            } else if (v == 1) {
                inout_gamepad->dpad |= 0x01;
            } else if (v == -1) {
                inout_gamepad->dpad |= 0x02;
            }

            break;
        }
        case GAMEPAD_DPAD_Y: {
            const int8_t v = msg_payload->status.dpad;

            inout_gamepad->dpad &= 0x0F;
            if (v == 0) {
                inout_gamepad->dpad |= 0x00;
            } else if (v == 1) {
                inout_gamepad->dpad |= 0x20;
            } else if (v == -1) {
                inout_gamepad->dpad |= 0x10;
            }

            break;
        }
        case GAMEPAD_GYROSCOPE: {
            if (msg_payload->status.gyro.sample_time.tv_sec != lastsec) {
                //printf("%d\n", numpackets);
                lastsec = msg_payload->status.gyro.sample_time.tv_sec;
                numpackets = 0;
            } else {
                ++numpackets;
            }
            inout_gamepad->last_gyro_motion_time = msg_payload->status.gyro.sample_time;
            inout_gamepad->raw_gyro[0] = msg_payload->status.gyro.x;
            inout_gamepad->raw_gyro[1] = msg_payload->status.gyro.y;
            inout_gamepad->raw_gyro[2] = msg_payload->status.gyro.z;
            break;
        }
        case GAMEPAD_ACCELEROMETER: {
            inout_gamepad->last_accel_motion_time = msg_payload->status.accel.sample_time;
            inout_gamepad->raw_accel[0] = msg_payload->status.accel.x;
            inout_gamepad->raw_accel[1] = msg_payload->status.accel.y;
            inout_gamepad->raw_accel[2] = msg_payload->status.accel.z;
            break;
        }
        default: {
            fprintf(stderr, "Unknown gamepad element: %d\n", msg_payload->element);
            break;
        }
    }
}

static void handle_incoming_message_keyboard_set(
    const dev_out_settings_t *const in_settings,
    const in_message_keyboard_set_element_t *const msg_payload,
    keyboard_status_t *const inout_kbd
) {
    switch (msg_payload->type) {
        case KEYBOARD_KEY_Q:
            inout_kbd->q = msg_payload->value;
            break;
        case KEYBOARD_KEY_W:
            inout_kbd->w = msg_payload->value;
            break;
        case KEYBOARD_KEY_E:
            inout_kbd->e = msg_payload->value;
            break;
        case KEYBOARD_KEY_R:
            inout_kbd->r = msg_payload->value;
            break;
        case KEYBOARD_KEY_T:
            inout_kbd->t = msg_payload->value;
            break;
        case KEYBOARD_KEY_Y:
            inout_kbd->y = msg_payload->value;
            break;
        case KEYBOARD_KEY_U:
            inout_kbd->u = msg_payload->value;
            break;
        case KEYBOARD_KEY_I:
            inout_kbd->i = msg_payload->value;
            break;
        case KEYBOARD_KEY_O:
            inout_kbd->o = msg_payload->value;
            break;
        case KEYBOARD_KEY_P:
            inout_kbd->p = msg_payload->value;
            break;
        case KEYBOARD_KEY_A:
            inout_kbd->a = msg_payload->value;
            break;
        case KEYBOARD_KEY_S:
            inout_kbd->s = msg_payload->value;
            break;
        case KEYBOARD_KEY_D:
            inout_kbd->d = msg_payload->value;
            break;
        case KEYBOARD_KEY_F:
            inout_kbd->f = msg_payload->value;
            break;
        case KEYBOARD_KEY_G:
            inout_kbd->g = msg_payload->value;
            break;
        case KEYBOARD_KEY_H:
            inout_kbd->h = msg_payload->value;
            break;
        case KEYBOARD_KEY_J:
            inout_kbd->j = msg_payload->value;
            break;
        case KEYBOARD_KEY_K:
            inout_kbd->k = msg_payload->value;
            break;
        case KEYBOARD_KEY_L:
            inout_kbd->l = msg_payload->value;
            break;
        case KEYBOARD_KEY_Z:
            inout_kbd->z = msg_payload->value;
            break;
        case KEYBOARD_KEY_X:
            inout_kbd->x = msg_payload->value;
            break;
        case KEYBOARD_KEY_C:
            inout_kbd->c = msg_payload->value;
            break;
        case KEYBOARD_KEY_V:
            inout_kbd->v = msg_payload->value;
            break;
        case KEYBOARD_KEY_B:
            inout_kbd->b = msg_payload->value;
            break;
        case KEYBOARD_KEY_N:
            inout_kbd->n = msg_payload->value;
            break;
        case KEYBOARD_KEY_M:
            inout_kbd->m = msg_payload->value;
            break;
        case KEYBOARD_KEY_UP:
            inout_kbd->up = msg_payload->value;
            break;
        case KEYBOARD_KEY_DOWN:
            inout_kbd->down = msg_payload->value;
            break;
        case KEYBOARD_KEY_LEFT:
            inout_kbd->left = msg_payload->value;
            break;
        case KEYBOARD_KEY_RIGHT:
            inout_kbd->right = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_0:
            inout_kbd->num_0 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_1:
            inout_kbd->num_1 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_2:
            inout_kbd->num_2 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_3:
            inout_kbd->num_3 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_4:
            inout_kbd->num_4 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_5:
            inout_kbd->num_5 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_6:
            inout_kbd->num_6 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_7:
            inout_kbd->num_7 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_8:
            inout_kbd->num_8 = msg_payload->value;
            break;
        case KEYBOARD_KEY_NUM_9:
            inout_kbd->num_9 = msg_payload->value;
            break;
        case KEYBOARD_KEY_LCRTL:
            inout_kbd->lctrl = msg_payload->value;
            break;
        default:
            fprintf(stderr, "key not implemented\n");
    }
}

static void handle_incoming_message(
    const dev_out_settings_t *const in_settings,
    const in_message_t *const msg,
    devices_status_t *const dev_stats
) {
    if (msg->type == GAMEPAD_SET_ELEMENT) {
        handle_incoming_message_gamepad_set(
            in_settings,
            &msg->data.gamepad_set,
            &dev_stats->gamepad
        );
    } else if (msg->type == GAMEPAD_ACTION) {
        handle_incoming_message_gamepad_action(
            in_settings,
            &msg->data.action,
            &dev_stats->gamepad
        );
    } else if (msg->type == MOUSE_EVENT) {
        handle_incoming_message_mouse_event(
            in_settings,
            &msg->data.mouse_event,
            &dev_stats->mouse
        );
    } else if (msg->type == KEYBOARD_SET_ELEMENT) {
        handle_incoming_message_keyboard_set(
            in_settings,
            &msg->data.kbd_set,
            &dev_stats->kbd
        );
    }
}

int64_t get_timediff_usec(const struct timespec *const start, const struct timespec *const end) {
    return (end->tv_sec - start->tv_sec) * 1000000000LL + (end->tv_nsec - start->tv_nsec);
}

void *dev_out_thread_func(void *ptr) {
    dev_out_data_t *const dev_out_data = (dev_out_data_t*)ptr;

    // Initialize device
    devices_status_init(&dev_out_data->dev_stats);

    dev_out_gamepad_device_t current_gamepad = GAMEPAD_DUALSENSE;
    
    switch (dev_out_data->settings.default_gamepad) {
        case 1:
            current_gamepad = GAMEPAD_DUALSENSE;
            break;
        case 2:
            current_gamepad = GAMEPAD_DUALSHOCK;
            break;

        default:
            current_gamepad = GAMEPAD_DUALSENSE;
            break;
    }

    int current_gamepad_fd = -1;
    int current_keyboard_fd = -1;
    int current_mouse_fd = -1;

    union {
        virt_dualshock_t ds4;
        virt_dualsense_t ds5;
    } controller_data;

    virt_mouse_t mouse_data;
    const int mouse_init_res = virt_mouse_init(&mouse_data);
    if (mouse_init_res < 0) {
        fprintf(stderr, "Unable to initialize virtual mouse -- will continue regardless\n");
    } else {
        current_mouse_fd = virt_mouse_get_fd(&mouse_data);
        printf("Mouse initialized: fd=%d\n", current_mouse_fd);
    }

    virt_kbd_t keyboard_data;
    const int kbd_init_res = virt_kbd_init(&keyboard_data);
    if (kbd_init_res < 0) {
        fprintf(stderr, "Unable to initialize virtual keyboard -- will continue regardless\n");
    } else {
        current_keyboard_fd = virt_kbd_get_fd(&keyboard_data);
        printf("Keyboard initialized: fd=%d\n", current_keyboard_fd);
    }

    const int64_t kbd_report_timing_us = 1125;
    const int64_t mouse_report_timing_us = 950;
    const int64_t gamepad_report_timing_us = 1250;

    if (current_gamepad == GAMEPAD_DUALSENSE) {
        const int ds5_init_res = virt_dualsense_init(&controller_data.ds5, true);
        if (ds5_init_res != 0) {
            fprintf(stderr, "Unable to initialize the DualSense device: %d\n", ds5_init_res);
        } else {
            current_gamepad_fd = virt_dualsense_get_fd(&controller_data.ds5);
            printf("DualSense initialized: fd=%d\n", current_gamepad_fd);
        }
    } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
        const int ds4_init_res = virt_dualshock_init(&controller_data.ds4);
        if (ds4_init_res != 0) {
            fprintf(stderr, "Unable to initialize the DualShock device: %d\n", ds4_init_res);
        } else {
            current_gamepad_fd = virt_dualshock_get_fd(&controller_data.ds4);
            printf("DualShock initialized: fd=%d\n", current_gamepad_fd);
        }
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    struct timespec gamepad_last_hid_report_sent = now;
    struct timespec mouse_last_hid_report_sent = now;
    struct timespec keyboard_last_hid_report_sent = now;

    uint8_t tmp_buf[256];

    fd_set read_fds;
    for (;;) {
        if (dev_out_data->flags & DEV_OUT_FLAG_EXIT) {
            printf("Termination signal received -- exiting dev_out\n");
            break;
        }

        const int64_t gamepad_time_diff_usecs = get_timediff_usec(&gamepad_last_hid_report_sent, &now);
        const int64_t mouse_time_diff_usecs = get_timediff_usec(&mouse_last_hid_report_sent, &now);
        const int64_t kbd_time_diff_usecs = get_timediff_usec(&keyboard_last_hid_report_sent, &now);

        clock_gettime(CLOCK_MONOTONIC, &now);
        if ((current_gamepad_fd > 0) && (gamepad_time_diff_usecs >= gamepad_report_timing_us)) {
            gamepad_last_hid_report_sent = now;
            
            if (current_gamepad == GAMEPAD_DUALSENSE) {
                virt_dualsense_compose(&controller_data.ds5, &dev_out_data->dev_stats.gamepad, tmp_buf);
                virt_dualsense_send(&controller_data.ds5, tmp_buf);
            } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
                virt_dualshock_compose(&controller_data.ds4, &dev_out_data->dev_stats.gamepad, tmp_buf);
                virt_dualshock_send(&controller_data.ds4, tmp_buf);
            }
        }
        
        if ((current_mouse_fd > 0) && (mouse_time_diff_usecs >= mouse_report_timing_us)) {
            mouse_last_hid_report_sent = now;

            virt_mouse_send(&mouse_data, &dev_out_data->dev_stats.mouse, NULL);
            
            // reset mouse movements now
            dev_out_data->dev_stats.mouse.x = 0;
            dev_out_data->dev_stats.mouse.y = 0;
        }
        
        if ((current_keyboard_fd > 0) && (kbd_time_diff_usecs >= kbd_report_timing_us)) {
            keyboard_last_hid_report_sent = now;

            virt_kbd_send(&keyboard_data, &dev_out_data->dev_stats.kbd, NULL);
        }


        // once here no output device needs to send out its report
        FD_ZERO(&read_fds);

        if (dev_out_data->communication.type == ipc_unix_pipe) {
            FD_SET(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd, &read_fds);
        } else if (dev_out_data->communication.type == ipc_server_sockets) {
            if (pthread_mutex_lock(&dev_out_data->communication.endpoint.ssocket.mutex) == 0) {
                for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                    const int fd = dev_out_data->communication.endpoint.ssocket.clients[i];
                    if (fd > 0) {
                        FD_SET(fd, &read_fds);
                    }
                }
            
                pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
            }
        }

        if (current_mouse_fd > 0) {
            FD_SET(current_mouse_fd, &read_fds);
        }
        
        if (current_keyboard_fd > 0) {
            FD_SET(current_keyboard_fd, &read_fds);
        }

        if (current_gamepad_fd > 0) {
            FD_SET(current_gamepad_fd, &read_fds);
        }

        const int64_t timeout_gamepad_time_diff_usecs = (current_gamepad_fd > 0) ? gamepad_report_timing_us - gamepad_time_diff_usecs : 5000;
        const int64_t timeout_mouse_time_diff_usecs = (current_mouse_fd > 0) ? mouse_report_timing_us - mouse_time_diff_usecs : 5000;
        const int64_t timeout_kbd_time_diff_usecs = (current_keyboard_fd > 0) ? kbd_report_timing_us - kbd_time_diff_usecs : 5000;

        int64_t next_timing_out_device_diff_usecs = 5000;
        
        if ((timeout_kbd_time_diff_usecs > 0) && (timeout_kbd_time_diff_usecs < next_timing_out_device_diff_usecs)) {
            next_timing_out_device_diff_usecs = timeout_kbd_time_diff_usecs;
        }
        
        if ((timeout_mouse_time_diff_usecs > 0) && (timeout_mouse_time_diff_usecs < next_timing_out_device_diff_usecs)) {
            next_timing_out_device_diff_usecs = timeout_mouse_time_diff_usecs;
        }

        if ((timeout_gamepad_time_diff_usecs > 0) && (timeout_gamepad_time_diff_usecs < next_timing_out_device_diff_usecs)) {
            next_timing_out_device_diff_usecs = timeout_gamepad_time_diff_usecs;
        }

        // calculate the shortest timeout between one of the multiple device will needs to send out its hid report
        struct timeval timeout = {
            .tv_sec = (__time_t)next_timing_out_device_diff_usecs / (__time_t)1000000,
            .tv_usec = (__suseconds_t)next_timing_out_device_diff_usecs % (__suseconds_t)1000000,
        };

        int ready_fds = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);
        gamepad_status_qam_quirk_ext_time(&dev_out_data->dev_stats.gamepad);

        if (ready_fds == -1) {
            const int err = errno;
            fprintf(stderr, "Error reading events for output devices: %d\n", err);
            usleep(1000);
            continue;
        } else if (ready_fds == 0) {
            // timeout: do nothing but continue. next iteration will take care
            continue;
        }

        if ((current_gamepad_fd > 0) && (FD_ISSET(current_gamepad_fd, &read_fds))) {
            const uint64_t prev_leds_events_count = dev_out_data->dev_stats.gamepad.leds_events_count;
            const uint64_t prev_motors_events_count = dev_out_data->dev_stats.gamepad.rumble_events_count;

            out_message_t out_msgs[4];
            size_t out_msgs_count = 0;
            if (current_gamepad == GAMEPAD_DUALSENSE) {
                virt_dualsense_event(&controller_data.ds5, &dev_out_data->dev_stats.gamepad);
            } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
                virt_dualshock_event(&controller_data.ds4, &dev_out_data->dev_stats.gamepad);
            }

            const uint64_t current_leds_events_count = dev_out_data->dev_stats.gamepad.leds_events_count;
            const uint64_t current_motors_events_count = dev_out_data->dev_stats.gamepad.rumble_events_count;

            if (current_leds_events_count != prev_leds_events_count) {
                const out_message_t msg = {
                    .type = OUT_MSG_TYPE_LEDS,
                    .data = {
                        .leds = {
                            .r = dev_out_data->dev_stats.gamepad.leds_colors[0],
                            .g = dev_out_data->dev_stats.gamepad.leds_colors[1],
                            .b = dev_out_data->dev_stats.gamepad.leds_colors[2],
                        }
                    }
                };

                if (dev_out_data->settings.gamepad_leds_control) {
                    out_msgs[out_msgs_count++] = msg;
                }
            }

            if (current_motors_events_count != prev_motors_events_count) {
                const out_message_t msg = {
                    .type = OUT_MSG_TYPE_RUMBLE,
                    .data = {
                        .rumble = {
                            .motors_left = dev_out_data->dev_stats.gamepad.motors_intensity[0],
                            .motors_right = dev_out_data->dev_stats.gamepad.motors_intensity[1],
                        }
                    }
                };

                if (dev_out_data->settings.gamepad_rumble_control) {
                    out_msgs[out_msgs_count++] = msg;
                }
            }

            // send out game-generated events to sockets
            if (dev_out_data->communication.type == ipc_unix_pipe) {
                for (int msg_idx = 0; msg_idx < out_msgs_count; ++msg_idx) {
                    const int write_res = write(dev_out_data->communication.endpoint.pipe.out_message_pipe_fd, (void*)&out_msgs[msg_idx], sizeof(out_message_t));
                    if (write_res != sizeof(out_message_t)) {
                        fprintf(stderr, "Error in writing out_message to out_message_pipe: %d\n", write_res);
                    }
                }
            } else if (dev_out_data->communication.type == ipc_server_sockets) {
                if (pthread_mutex_lock(&dev_out_data->communication.endpoint.ssocket.mutex) == 0) {
                    for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                        if (dev_out_data->communication.endpoint.ssocket.clients[i] > 0) {
                            for (int msg_idx = 0; msg_idx < out_msgs_count; ++msg_idx) {
                                const int write_res = write(dev_out_data->communication.endpoint.ssocket.clients[i], (void*)&out_msgs[msg_idx], sizeof(out_message_t));
                                if (write_res != sizeof(out_message_t)) {
                                    fprintf(stderr, "Error in writing out_message to socket number %d: %d\n", i, write_res);
                                    close(dev_out_data->communication.endpoint.ssocket.clients[i]);
                                    dev_out_data->communication.endpoint.ssocket.clients[i] = -1;
                                }
                            }
                        }
                    }

                    pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
                }
            }
        }

        if ((current_keyboard_fd > 0) && (FD_ISSET(current_keyboard_fd, &read_fds))) {
            // TODO: read keyboard events
        }

        if ((current_mouse_fd > 0) && (FD_ISSET(current_mouse_fd, &read_fds))) {
            // TODO: read mouse events
        }

        // read and handle incoming data: this data is packed into in_message_t
        if (dev_out_data->communication.type == ipc_unix_pipe) {
            if (FD_ISSET(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd, &read_fds)) {
                in_message_t incoming_message;
                const size_t in_message_pipe_read_res = read(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd, (void*)&incoming_message, sizeof(in_message_t));
                if (in_message_pipe_read_res == sizeof(in_message_t)) {
                    handle_incoming_message(
                        &dev_out_data->settings,
                        &incoming_message,
                        &dev_out_data->dev_stats
                    );
                } else {
                    fprintf(stderr, "Error reading from in_message_pipe_fd: got %zu bytes, expected %zu bytes\n", in_message_pipe_read_res, sizeof(in_message_t));
                }
            }
        } else if (dev_out_data->communication.type == ipc_server_sockets) {
            if (pthread_mutex_lock(&dev_out_data->communication.endpoint.ssocket.mutex) == 0) {
                for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                    const int fd = dev_out_data->communication.endpoint.ssocket.clients[i];
                    if ((fd > 0) && (FD_ISSET(fd, &read_fds))) {
                        in_message_t incoming_message;
                        const size_t in_message_pipe_read_res = read(fd, (void*)&incoming_message, sizeof(in_message_t));
                        if (in_message_pipe_read_res == sizeof(in_message_t)) {
                            handle_incoming_message(
                                &dev_out_data->settings,
                                &incoming_message,
                                &dev_out_data->dev_stats
                            );
                        } else {
                            fprintf(stderr, "Error reading from socket number %d: got %zu bytes, expected %zu bytes\n", i, in_message_pipe_read_res, sizeof(in_message_t));
                            close(dev_out_data->communication.endpoint.ssocket.clients[i]);
                            dev_out_data->communication.endpoint.ssocket.clients[i] = -1;
                        }
                    }
                }
            
                pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
            }
        }
    }

    // close the gamepad output device
    if (current_gamepad_fd > 0) {
        if (current_gamepad == GAMEPAD_DUALSENSE) {
            virt_dualsense_close(&controller_data.ds5);
        } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
            virt_dualshock_close(&controller_data.ds4);
        }
    }

    // close the mouse device
    if (current_mouse_fd > 0) {
        virt_mouse_close(&mouse_data);
    }

    // close the keyboard device
    if (current_keyboard_fd > 0) {
        virt_kbd_close(&keyboard_data);
    }

    // end communication
    if (dev_out_data->communication.type == ipc_server_sockets) {
        // close every client socket
        if (pthread_mutex_lock(&dev_out_data->communication.endpoint.ssocket.mutex) == 0) {
            for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                close(dev_out_data->communication.endpoint.ssocket.clients[i]);
                dev_out_data->communication.endpoint.ssocket.clients[i] = -1;
            }

            pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
        }
    } else if (dev_out_data->communication.type == ipc_unix_pipe) {
        close(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd);
        close(dev_out_data->communication.endpoint.pipe.out_message_pipe_fd);
    } else if (dev_out_data->communication.type == ipc_client_socket) {
        close(dev_out_data->communication.endpoint.socket.fd);
        dev_out_data->communication.endpoint.socket.fd = -1;
    }

    return NULL;
}
