#include "dev_out.h"

#include "devices_status.h"
#include "ipc.h"
#include "message.h"
#include "virt_ds4.h"
#include "virt_ds5.h"

static void handle_incoming_message_gamepad_action(
    const in_message_gamepad_action_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    if (*msg_payload == GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER) {
        inout_gamepad->flags |= GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
    } else if (*msg_payload == GAMEPAD_ACTION_OPEN_STEAM_QAM) {
        inout_gamepad->flags |= GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
    } 
}

static void handle_incoming_message_gamepad_set(
    const in_message_gamepad_set_element_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    switch (msg_payload->element) {
        case GAMEPAD_BTN_CROSS: {
            inout_gamepad->cross = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_CIRCLE: {
            inout_gamepad->circle = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_SQUARE: {
            inout_gamepad->square = msg_payload->status.btn;
            break;
        }
        case GAMEPAD_BTN_TRIANGLE: {
            inout_gamepad->triangle = msg_payload->status.btn;
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

static void handle_incoming_message(
    const in_message_t *const msg,
    devices_status_t *const dev_stats
) {
    if (msg->type == GAMEPAD_SET_ELEMENT) {
        handle_incoming_message_gamepad_set(&msg->data.gamepad_set, &dev_stats->gamepad);
    } else if (msg->type == GAMEPAD_ACTION) {
        handle_incoming_message_gamepad_action(&msg->data.action, &dev_stats->gamepad);
    }
}

int64_t get_timediff_usec(const struct timeval *const past, const struct timeval *const now) {
    struct timeval tdiff;
    timersub(now, past, &tdiff);

    //const int64_t sgn = ((now->tv_sec > past->tv_sec) || ((now->tv_sec == past->tv_sec) && (now->tv_usec > past->tv_usec))) ? -1 : +1;

    return (int64_t)(tdiff.tv_sec) * (int64_t)1000000 + (int64_t)(tdiff.tv_usec);
}

void *dev_out_thread_func(void *ptr) {
    dev_out_data_t *const dev_out_data = (dev_out_data_t*)ptr;

    // Initialize device
    devices_status_init(&dev_out_data->dev_stats);

    dev_out_gamepad_device_t current_gamepad = dev_out_data->gamepad;

    union {
        virt_dualshock_t ds4;
        virt_dualsense_t ds5;
    } controller_data;

    int current_gamepad_fd = -1;
    //int current_keyboard_fd = -1;
    //int current_mouse_fd = -1;

    if (current_gamepad == GAMEPAD_DUALSENSE) {
        const int ds5_init_res = virt_dualsense_init(&controller_data.ds5);
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

    struct timeval now = {0};
    gettimeofday(&now, NULL);

    struct timeval gamepad_last_hid_report_sent = now;
    //struct timeval mouse_last_hid_report_sent = now;
    //struct timeval keyboard_last_hid_report_sent = now;

    uint8_t tmp_buf[256];

    fd_set read_fds;
    for (;;) {
        if (dev_out_data->flags & DEV_OUT_FLAG_EXIT) {
            printf("Termination signal received -- exiting dev_out\n");
            break;
        }

        gettimeofday(&now, NULL);
        int64_t gamepad_time_diff_usecs = get_timediff_usec(&gamepad_last_hid_report_sent, &now);
        if (gamepad_time_diff_usecs >= 1250) {
            gamepad_last_hid_report_sent = now;
            
            if (current_gamepad == GAMEPAD_DUALSENSE) {
                virt_dualsense_compose(&controller_data.ds5, &dev_out_data->dev_stats.gamepad, tmp_buf);
                virt_dualsense_send(&controller_data.ds5, tmp_buf);
            } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
                virt_dualshock_compose(&controller_data.ds4, &dev_out_data->dev_stats.gamepad, tmp_buf);
                virt_dualshock_send(&controller_data.ds4, tmp_buf);
            }
        } else {
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

            // TODO: FD_SET(current_mouse_fd, &read_fds);
            // TODO: FD_SET(current_keyboard_fd, &read_fds);
            FD_SET(current_gamepad_fd, &read_fds);

            // calculate the shortest timeout between one of the multiple device will needs to send out its hid report
            struct timeval timeout = {
                .tv_sec = (__time_t)gamepad_time_diff_usecs / (__time_t)1000000,
                .tv_usec = (__suseconds_t)gamepad_time_diff_usecs % (__suseconds_t)1000000,
            };

            int ready_fds = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);
            gamepad_status_qam_quirk_ext_time(&dev_out_data->dev_stats.gamepad, &now);

            if (ready_fds == -1) {
                const int err = errno;
                fprintf(stderr, "Error reading events for output devices: %d\n", err);
                continue;
            } else if (ready_fds == 0) {
                // timeout: do nothing but continue. next iteration will take care
                continue;
            }

            
            if (FD_ISSET(current_gamepad_fd, &read_fds)) {
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

                    out_msgs[out_msgs_count++] = msg;
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

                    out_msgs[out_msgs_count++] = msg;
                }

                // send out game-generated events to sockets
                int fd = -1;
                const size_t bytes_to_send = sizeof(out_message_t) * out_msgs_count;

                if (dev_out_data->communication.type == ipc_unix_pipe) {
                    const int write_res = write(dev_out_data->communication.endpoint.pipe.out_message_pipe_fd, (void*)&out_msgs, bytes_to_send);
                    if (write_res != bytes_to_send) {
                        fprintf(stderr, "Error in writing out_message to out_message_pipe: %d\n", write_res);
                    }
                } else if (dev_out_data->communication.type == ipc_server_sockets) {
                    if (pthread_mutex_lock(&dev_out_data->communication.endpoint.ssocket.mutex) == 0) {
                        for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                            if (dev_out_data->communication.endpoint.ssocket.clients[i] > 0) {
                                const int write_res = write(dev_out_data->communication.endpoint.ssocket.clients[i], (void*)&out_msgs, bytes_to_send);
                                if (write_res != bytes_to_send) {
                                    fprintf(stderr, "Error in writing out_message to socket number %d: %d\n", i, write_res);
                                    close(dev_out_data->communication.endpoint.ssocket.clients[i]);
                                    dev_out_data->communication.endpoint.ssocket.clients[i] = -1;
                                }
                            }
                        }

                        pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
                    }
                }
            }

            // read and handle incoming data: this data is packed into in_message_t
            if (dev_out_data->communication.type == ipc_unix_pipe) {
                if (FD_ISSET(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd, &read_fds)) {
                    in_message_t incoming_message;
                    const size_t in_message_pipe_read_res = read(dev_out_data->communication.endpoint.pipe.in_message_pipe_fd, (void*)&incoming_message, sizeof(in_message_t));
                    if (in_message_pipe_read_res == sizeof(in_message_t)) {
                        handle_incoming_message(&incoming_message, &dev_out_data->dev_stats);
                    } else {
                        fprintf(stderr, "Error reading from in_message_pipe_fd: got %zu bytes, expected %zu butes\n", in_message_pipe_read_res, sizeof(in_message_t));
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
                                handle_incoming_message(&incoming_message, &dev_out_data->dev_stats);
                            } else {
                                fprintf(stderr, "Error reading from socket number %d: got %zu bytes, expected %zu butes\n", i, in_message_pipe_read_res, sizeof(in_message_t));
                                close(dev_out_data->communication.endpoint.ssocket.clients[i]);
                                dev_out_data->communication.endpoint.ssocket.clients[i] = -1;
                            }
                        }
                    }
                
                    pthread_mutex_unlock(&dev_out_data->communication.endpoint.ssocket.mutex);
                }
            }

            
        }
    }

    // close the output device
    if (current_gamepad == GAMEPAD_DUALSENSE) {
        virt_dualsense_close(&controller_data.ds5);
    } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
        virt_dualshock_close(&controller_data.ds4);
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
