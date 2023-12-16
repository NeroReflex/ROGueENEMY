#include "dev_in.h"
#include "dev_hidraw.h"
#include "input_dev.h"
#include "ipc.h"
#include "message.h"
#include "dev_evdev.h"
#include "dev_iio.h"

#include <libconfig.h>

typedef enum dev_in_type {
    DEV_IN_TYPE_NONE,
    DEV_IN_TYPE_HIDRAW,
    DEV_IN_TYPE_IIO,
    DEV_IN_TYPE_EV,
} dev_in_type_t;

typedef struct dev_in_iio {

    dev_iio_t *iiodev;

} dev_in_iio_t;

typedef struct dev_in_hidraw {
    dev_hidraw_t *hidrawdev;

    hidraw_callbacks_t callbacks;

    void* user_data;
} dev_in_hidraw_t;

typedef struct dev_in_ev {
    bool ignore_msc_scan;

    bool ignore_timestamp;

    struct libevdev* evdev;

    bool grabbed;

    bool has_syn_report;

    bool has_rumble_support;

    struct ff_effect ff_effect;

} dev_in_ev_t;

typedef union dev_in_aggr {
    dev_in_ev_t evdev;
    dev_in_iio_t iio;
    dev_in_hidraw_t hidraw;
} dev_in_aggr_t;

typedef struct dev_in {

    dev_in_type_t type;

    dev_in_aggr_t dev;

} dev_in_t;

static int map_message_from_iio(dev_in_iio_t *const in_iio, in_message_t *const messages, size_t messages_len) {
    int res = -EIO;

    uint8_t data[32];

    struct timeval read_time;
    gettimeofday(&read_time, NULL);

    res = read(dev_iio_get_buffer_fd(in_iio->iiodev), &data[0], sizeof(data));
    if (res == -1) {
        res = errno;
        res = res > 0 ? -1 * res : res;
        goto send_message_from_iio_err;
    }

    if (res != 24) {
        fprintf(stderr, "Invalid read lenght\n");
        res = -EIO;
        goto send_message_from_iio_err;
    }

    uint16_t *const scan_elements = (uint16_t*)&data[0];

    messages[0].type = GAMEPAD_SET_ELEMENT;
    messages[0].data.gamepad_set.element = GAMEPAD_ACCELEROMETER;
    messages[0].data.gamepad_set.status.accel.sample_time = read_time;
    //messages[0].data.gamepad_set.status.accel.x = scan_elements[0];
    //messages[0].data.gamepad_set.status.accel.y = scan_elements[1];
    //messages[0].data.gamepad_set.status.accel.z = scan_elements[2];
    messages[0].data.gamepad_set.status.accel.x = scan_elements[0];
    messages[0].data.gamepad_set.status.accel.y = (uint16_t)(-1) * scan_elements[2];
    messages[0].data.gamepad_set.status.accel.z = (uint16_t)(-1) * scan_elements[1];

    messages[1].type = GAMEPAD_SET_ELEMENT;
    messages[1].data.gamepad_set.element = GAMEPAD_GYROSCOPE;
    messages[1].data.gamepad_set.status.gyro.sample_time = read_time;
    //messages[1].data.gamepad_set.status.gyro.x = scan_elements[3];
    //messages[1].data.gamepad_set.status.gyro.y = scan_elements[4];
    //messages[1].data.gamepad_set.status.gyro.z = scan_elements[5];
    messages[1].data.gamepad_set.status.gyro.x = scan_elements[3];
    messages[1].data.gamepad_set.status.gyro.y = (uint16_t)(-1) * scan_elements[5];
    messages[1].data.gamepad_set.status.gyro.z = (uint16_t)(-1) * scan_elements[4];

    res = 2;

send_message_from_iio_err:
    return res;
}

static int fill_message_from_evdev(dev_in_ev_t *const in_evdev, evdev_collected_t *const out_coll) {
    int res = 0;

    struct input_event read_ev;

    // reset the events count
    out_coll->ev_count = 0;

    // if the device does not support syn reports read just one event and return
    if (!in_evdev->has_syn_report) {
        res = libevdev_next_event(in_evdev->evdev, LIBEVDEV_READ_FLAG_NORMAL, &read_ev);
        if (res < 0) {
            goto fill_message_from_evdev_err;
        }

        // just copy the input event
        out_coll->ev[out_coll->ev_count++] = read_ev;
        
        goto fill_message_from_evdev_err_completed;
    }

    // the device does support syn reports so read every event until one is found
    while (out_coll->ev_count < MAX_COLLECTED_EVDEV_EVENTS) {
        res = libevdev_next_event(in_evdev->evdev, LIBEVDEV_READ_FLAG_BLOCKING, &read_ev);
        if (res < 0) {
            goto fill_message_from_evdev_err;
        }

        // if the message is a syn_report exit the while as there are no more events
        if ((read_ev.type == EV_SYN) && (read_ev.code == SYN_REPORT)) {
            break;
        } else if ((in_evdev->ignore_msc_scan) && (read_ev.type == EV_MSC) && (read_ev.code == MSC_SCAN)) {
            continue;
        } else if ((in_evdev->ignore_timestamp) && (read_ev.type == EV_MSC) && (read_ev.code == MSC_TIMESTAMP)) {
            continue;
        }

        // just copy the input event
        out_coll->ev[out_coll->ev_count++] = read_ev;
    }

fill_message_from_evdev_err:
fill_message_from_evdev_err_completed:
    return res;
}

static int hidraw_open_device(
    const dev_in_settings_t *const in_settings,
    const hidraw_filters_t *const in_filters,
    dev_in_hidraw_t *const out_dev
) {
    int res = dev_hidraw_open(in_filters, &out_dev->hidrawdev);
    if (res != 0) {
        fprintf(stderr, "Unable to open the specified iio device: %d\n", res);
        goto iio_open_device_err;
    }

    printf(
        "Opened hidraw device: %x:%x\n",
        dev_hidraw_get_vid(out_dev->hidrawdev),
        dev_hidraw_get_pid(out_dev->hidrawdev)
    );

iio_open_device_err:
    return res;
}

static int iio_open_device(
    const dev_in_settings_t *const in_settings,
    const iio_filters_t *const in_filters,
    dev_in_iio_t *const out_dev
) {
    int res = dev_iio_open(in_filters, &out_dev->iiodev);
    if (res != 0) {
        fprintf(stderr, "Unable to open the specified iio device: %d\n", res);
        goto iio_open_device_err;
    }

    const char *const dev_name = dev_iio_get_name(out_dev->iiodev);

    printf(
        "Opened iio device:\n   name: %s\n    has accel: %s\n    has anglvel: %s\n",
        (dev_name != NULL) ? dev_name : "NULL",
        dev_iio_has_accel(out_dev->iiodev) ? "yes" : "no",
        dev_iio_has_anglvel(out_dev->iiodev) ? "yes" : "no"
    );

iio_open_device_err:
    return res;
}

static int evdev_open_device(
    const dev_in_settings_t *const in_settings,
    const uinput_filters_t *const in_filters,
    dev_in_ev_t *const out_dev
) {
    int res = dev_evdev_open(in_filters, &out_dev->evdev);
    if (res != 0) {
        fprintf(stderr, "Unable to open the specified ev device: %d\n", res);
        goto evdev_open_device_err;
    }

    out_dev->ignore_timestamp = true;
    out_dev->ignore_msc_scan = false;
    out_dev->has_rumble_support = libevdev_has_event_type(out_dev->evdev, EV_FF) && libevdev_has_event_code(out_dev->evdev, EV_FF, FF_RUMBLE);
    out_dev->has_syn_report = libevdev_has_event_type(out_dev->evdev, EV_SYN) && libevdev_has_event_code(out_dev->evdev, EV_SYN, SYN_REPORT);

    const char *const dev_name = libevdev_get_name(out_dev->evdev);

    const int grab_res = libevdev_grab(out_dev->evdev, LIBEVDEV_GRAB);
    out_dev->grabbed = grab_res == 0;
    if (!out_dev->grabbed) {
        fprintf(stderr, "Unable to grab the device (%s): %d.\n", dev_name == NULL ? "NULL" : dev_name, grab_res);
    }

    if (out_dev->has_rumble_support) {
        printf("Opened ev device\n    name: %s\n    rumble: %s\n",
            dev_name == NULL ? "NULL" : dev_name,
            libevdev_has_event_code(out_dev->evdev, EV_FF, FF_RUMBLE) ? "true" : "false"
        );

        // prepare the rumble effect
        out_dev->ff_effect.type = FF_RUMBLE;
        out_dev->ff_effect.id = -1;
        out_dev->ff_effect.replay.delay = 0;
        out_dev->ff_effect.replay.length = 1000;
        out_dev->ff_effect.u.rumble.strong_magnitude = 0x0000;
        out_dev->ff_effect.u.rumble.weak_magnitude = 0x0000;

        const struct input_event gain = {
            .type = EV_FF,
            .code = FF_GAIN,
            .value = in_settings->ff_gain,
        };

        const int gain_set_res = write(libevdev_get_fd(out_dev->evdev), (const void*)&gain, sizeof(gain));
        if (gain_set_res != sizeof(gain)) {
            fprintf(stderr, "Unable to adjust gain for force-feedback: %d\n", gain_set_res);
        }
    } else {
        printf("Opened device\n    name: %s\n    rumble: no force-feedback\n",
            dev_name == NULL ? "NULL" : dev_name
        );
    }

evdev_open_device_err:
    return res;
}

static void evdev_close_device(dev_in_ev_t *const out_dev) {
    dev_evdev_close(out_dev->evdev);
}

static void iio_close_device(dev_in_iio_t *const out_dev) {
    dev_iio_close(out_dev->iiodev);
}

static void hidraw_close_device(dev_in_hidraw_t *const out_hidraw) {
    dev_hidraw_close(out_hidraw->hidrawdev);
}

static void handle_rumble_device(const dev_in_settings_t *const conf, dev_in_ev_t *const in_dev, const out_message_rumble_t *const in_rumble_msg) {
    if (!in_dev->has_rumble_support) {
        return;
    }

    int fd = libevdev_get_fd(in_dev->evdev);

    // here stop the previous rumble
    if (in_dev->ff_effect.id != -1) {
        struct input_event rumble_stop = {
            .type = EV_FF,
            .code = in_dev->ff_effect.id,
            .value = 0,
        };

        const int rumble_stop_res = write(fd, (const void*) &rumble_stop, sizeof(rumble_stop));
        if (rumble_stop_res != sizeof(rumble_stop)) {
            fprintf(stderr, "Unable to stop the previous rumble: %d\n", rumble_stop_res);
        }
    }

    // load the new effect data
    in_dev->ff_effect.u.rumble.strong_magnitude = div_round_closest((int32_t)0xFFFF*(int32_t)in_rumble_msg->motors_left, (int32_t)0xFF);
    in_dev->ff_effect.u.rumble.weak_magnitude = div_round_closest((int32_t)0xFFFF*(int32_t)in_rumble_msg->motors_right, (int32_t)0xFF);

    // upload the new effect to the device
    const int effect_upload_res = ioctl(fd, EVIOCSFF, &in_dev->ff_effect);
    if (effect_upload_res == 0) {
        const struct input_event rumble_play = {
            .type = EV_FF,
            .code = in_dev->ff_effect.id,
            .value = 1,
        };

        const int effect_start_res = write(fd, (const void*)&rumble_play, sizeof(rumble_play));
        if (effect_start_res == sizeof(rumble_play)) {
#if defined(INCLUDE_INPUT_DEBUG)
            printf("Rumble effect play requested to driver\n");
#endif
        } else {
            fprintf(stderr, "Unable to write input event starting the rumble: %d\n", effect_start_res);
        }
    } else {
        fprintf(stderr, "Unable to update force-feedback effect: %d\n", effect_upload_res);

        in_dev->ff_effect.id = -1;
    }
}

static void handle_rumble(const dev_in_settings_t *const conf, dev_in_t *const in_devs, size_t in_devs_count, const out_message_rumble_t *const in_rumble_msg) {
    for (size_t i = 0; i < in_devs_count; ++i) {
        if (in_devs[i].type == DEV_IN_TYPE_EV) {
            handle_rumble_device(
                conf,
                &in_devs[i].dev.evdev,
                in_rumble_msg
            );
        }
    }
}

static void handle_leds(const dev_in_settings_t *const conf, dev_in_t *const in_devs, size_t in_devs_count, const out_message_leds_t *const in_leds_msg) {
    for (size_t i = 0; i < in_devs_count; ++i) {
        if (in_devs[i].type == DEV_IN_TYPE_HIDRAW) {
            in_devs[i].dev.hidraw.callbacks.leds_callback(
                conf,
                in_devs[i].dev.hidraw.hidrawdev->fd,
                in_leds_msg->r,
                in_leds_msg->g,
                in_leds_msg->b,
                in_devs[i].dev.hidraw.user_data
            );
        }
    }
}

static int open_socket(struct sockaddr_un *serveraddr) {
    int res = -ENODEV;

    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sd < 0)
    {
        res = sd;
        goto open_socket_err;
    }

    res = connect(sd, (struct sockaddr *)serveraddr, SUN_LEN(serveraddr));
    if (res < 0) {
        goto open_socket_err;
    }

    res = sd;

open_socket_err:
    return res;
}

void* dev_in_thread_func(void *ptr) {
    dev_in_data_t *const dev_in_data = (dev_in_data_t*)ptr;

    void* platform_data;
    const int platform_init_res = dev_in_data->input_dev_decl->init_fn(&dev_in_data->settings, &platform_data);
    if (platform_init_res != 0) {
        fprintf(stderr, "Error setting up platform data: %d\n", platform_init_res);
    }

    struct timeval timeout = {
        .tv_sec = (__time_t)dev_in_data->timeout_ms / (__time_t)1000,
        .tv_usec = ((__suseconds_t)dev_in_data->timeout_ms % (__suseconds_t)1000) * (__suseconds_t)1000000,
    };

    fd_set read_fds;
    
    const size_t max_devices = dev_in_data->input_dev_decl->dev_count;

    dev_in_t* const devices = malloc(sizeof(dev_in_t) * max_devices);
    if (devices == NULL) {
        fprintf(stderr, "Unable to allocate memory to hold devices -- aborting input thread\n");
        return NULL;
    }

    // flag every device as disconnected
    for (size_t i = 0; i < max_devices; ++i) {
        devices[i].type = DEV_IN_TYPE_NONE;
    }

    for (;;) {
        if (dev_in_data->flags & DEV_IN_FLAG_EXIT) {
            printf("Termination signal received -- exiting dev_in\n");
            break;
        }

        FD_ZERO(&read_fds);

        if (dev_in_data->communication.type == ipc_unix_pipe) {
            FD_SET(dev_in_data->communication.endpoint.pipe.out_message_pipe_fd, &read_fds);
        } else if (dev_in_data->communication.type == ipc_client_socket) {
            // only reconnect if the fd is invalid
            if (dev_in_data->communication.endpoint.socket.fd < 0) {
                dev_in_data->communication.endpoint.socket.fd = open_socket(&dev_in_data->communication.endpoint.socket.serveraddr);

                // do not do a thing! that will consume messages and they won't be available anymore!
                if (dev_in_data->communication.endpoint.socket.fd < 0) {
                    fprintf(stderr, "Unable to connect to server: %d -- will retry connection\n", dev_in_data->communication.endpoint.socket.fd);
                    usleep(500000);
                    continue;
                }
            }

            FD_SET(dev_in_data->communication.endpoint.socket.fd, &read_fds);
        }

        for (size_t i = 0; i < max_devices; ++i) {
            if (devices[i].type == DEV_IN_TYPE_EV) {
                // device is present, query it in select
                FD_SET(libevdev_get_fd(devices[i].dev.evdev.evdev), &read_fds);
            } else if (devices[i].type == DEV_IN_TYPE_IIO) {
                FD_SET(dev_iio_get_buffer_fd(devices[i].dev.iio.iiodev), &read_fds);
            } else if (devices[i].type == DEV_IN_TYPE_HIDRAW) {
                FD_SET(dev_hidraw_get_fd(devices[i].dev.hidraw.hidrawdev), &read_fds);
            } else if (devices[i].type == DEV_IN_TYPE_NONE) {
                const input_dev_type_t d_type = dev_in_data->input_dev_decl->dev[i]->dev_type;
                if (d_type == input_dev_type_uinput) {
                    fprintf(stderr, "Device (evdev) %zu not found -- Attempt reconnection for device named %s\n", i, dev_in_data->input_dev_decl->dev[i]->filters.ev.name);

                    const int open_res = evdev_open_device(
                        &dev_in_data->settings,
                        &dev_in_data->input_dev_decl->dev[i]->filters.ev,
                        &devices[i].dev.evdev
                    );
                    if (open_res == 0) {
                        devices[i].type = DEV_IN_TYPE_EV;

                        // device is now connected, query it in select
                        FD_SET(libevdev_get_fd(devices[i].dev.evdev.evdev), &read_fds);
                    }
                } else if (d_type == input_dev_type_iio) {
                    fprintf(stderr, "Device (iio) %zu not found -- Attempt reconnection for device named %s\n", i, dev_in_data->input_dev_decl->dev[i]->filters.iio.name);

                    const int open_res = iio_open_device(
                        &dev_in_data->settings,
                        &dev_in_data->input_dev_decl->dev[i]->filters.iio,
                        &devices[i].dev.iio
                    );

                    if (open_res == 0) {
                        devices[i].type = DEV_IN_TYPE_IIO;
                    }
                } else if (d_type == input_dev_type_hidraw) {
                    fprintf(stderr, "Device (hidraw) %zu not found -- Attempt reconnection for device %x:%x\n", i, dev_in_data->input_dev_decl->dev[i]->filters.hidraw.pid, dev_in_data->input_dev_decl->dev[i]->filters.hidraw.vid);

                    const int open_res = hidraw_open_device(
                        &dev_in_data->settings,
                        &dev_in_data->input_dev_decl->dev[i]->filters.hidraw,
                        &devices[i].dev.hidraw
                    );

                    if (open_res == 0) {
                        devices[i].dev.hidraw.callbacks = dev_in_data->input_dev_decl->dev[i]->map.hidraw_callbacks;
                        devices[i].dev.hidraw.user_data = dev_in_data->input_dev_decl->dev[i]->user_data;
                        devices[i].type = DEV_IN_TYPE_HIDRAW;
                    }
                }
            }
        }

        int ready_fds = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);

        if (ready_fds == -1) {
            const int err = errno;
            fprintf(stderr, "Error reading devices: %d\n", err);
            continue;
        } else if (ready_fds == 0) {
            // Timeout... simply retry
            continue;
        }

        // check for messages incoming like set leds or activate rumble
        int out_message_fd = -1;
        if (dev_in_data->communication.type == ipc_unix_pipe) {
            out_message_fd = dev_in_data->communication.endpoint.pipe.out_message_pipe_fd;
        } else if (dev_in_data->communication.type == ipc_client_socket) {
            out_message_fd = dev_in_data->communication.endpoint.socket.fd;
        }

        if (FD_ISSET(out_message_fd, &read_fds)) {
            out_message_t out_msg;
            const ssize_t out_message_pipe_read_res = read(out_message_fd, (void*)&out_msg, sizeof(out_message_t));
            if (out_message_pipe_read_res == sizeof(out_message_t)) {
                if (out_msg.type == OUT_MSG_TYPE_RUMBLE) {
                    handle_rumble(&dev_in_data->settings, devices, max_devices, &out_msg.data.rumble);
                } else if (out_msg.type == OUT_MSG_TYPE_LEDS) {
                    // first inform the platform
                    const int platform_leds_res = dev_in_data->input_dev_decl->leds_fn(
                        &dev_in_data->settings,
                        out_msg.data.leds.r, out_msg.data.leds.g,
                        out_msg.data.leds.b, platform_data
                    );
                    if (platform_leds_res != 0) {
                        fprintf(stderr, "Error in changing platform LEDs: %d\n", platform_leds_res);
                    }

                    handle_leds(&dev_in_data->settings, devices, max_devices, &out_msg.data.leds);
                }
            } else {
                fprintf(stderr, "Error reading from out_message_pipe_fd: got %zu bytes, expected %zu bytes\n", out_message_pipe_read_res, sizeof(out_message_t));

                // in case of an error reschedule to socket for reconnection
                if (dev_in_data->communication.type == ipc_client_socket) {
                    close(out_message_fd);
                    dev_in_data->communication.endpoint.socket.fd = -1;
                }
            }
        }

        // the following is only executed when there is actual data in at least one of the fd
        for (size_t i = 0; i < max_devices; ++i) {
            int fd = -1;
            if (devices[i].type == DEV_IN_TYPE_EV) {
                fd = libevdev_get_fd(devices[i].dev.evdev.evdev);
            } else if (devices[i].type == DEV_IN_TYPE_IIO) {
                fd = dev_iio_get_buffer_fd(devices[i].dev.iio.iiodev);
            } else if (devices[i].type == DEV_IN_TYPE_HIDRAW) {
                fd = dev_hidraw_get_fd(devices[i].dev.hidraw.hidrawdev);
            } else {
                continue;
            }
            
            if (!FD_ISSET(fd, &read_fds)) {
                continue;
            }

            in_message_t controller_msg[MAX_IN_MESSAGES];
            size_t controller_msg_avail = sizeof(controller_msg) / sizeof(in_message_t);
            int controller_msg_count = -EIO;

            // the following part fills controller_msg and writes in controller_msg_count an error or the number of messages to be sent to the output device
            if (devices[i].type == DEV_IN_TYPE_EV) {
                evdev_collected_t coll = {
                    .ev_count = 0
                };

                controller_msg_count = fill_message_from_evdev(&devices[i].dev.evdev, &coll);
                if (controller_msg_count != 0) {
                    fprintf(stderr, "Unable to fill input_event(s) for device %zd: %d -- Will reconnect the device\n", i, controller_msg_count);
                    evdev_close_device(&devices[i].dev.evdev);
                    devices[i].type = DEV_IN_TYPE_NONE;
                    continue;
                }

                controller_msg_count = dev_in_data->input_dev_decl->dev[i]->map.ev_input_map_fn(
                    &dev_in_data->settings,
                    &coll,
                    &controller_msg[0],
                    controller_msg_avail,
                    dev_in_data->input_dev_decl->dev[i]->user_data
                );
            } else if (devices[i].type == DEV_IN_TYPE_IIO) {
                controller_msg_count = map_message_from_iio(
                    &devices[i].dev.iio,
                    &controller_msg[0],
                    controller_msg_avail
                );
                if (controller_msg_count < 0) {
                    fprintf(stderr, "Error in reading iio buffer for device %zd: %d -- Will reconnect to the device\n", i, controller_msg_count);
                    iio_close_device(&devices[i].dev.iio);
                    devices[i].type = DEV_IN_TYPE_NONE;
                    continue;
                }
            } else if (devices[i].type == DEV_IN_TYPE_HIDRAW) {
                controller_msg_count = dev_in_data->input_dev_decl->dev[i]->map.hidraw_callbacks.map_callback(
                    &dev_in_data->settings,
                    dev_hidraw_get_fd(devices[i].dev.hidraw.hidrawdev),
                    &controller_msg[0],
                    controller_msg_avail,
                    dev_in_data->input_dev_decl->dev[i]->user_data
                );
                if (controller_msg_count < 0) {
                    fprintf(stderr, "Error in performing operations for device %zd: %d -- Will reconnect to the device\n", i, controller_msg_count);
                    hidraw_close_device(&devices[i].dev.hidraw);
                    devices[i].type = DEV_IN_TYPE_NONE;
                    continue;
                }
            }

            // send messages (if any)
            if (controller_msg_count <= 0) {
                continue;
            }

            if (dev_in_data->communication.type == ipc_client_socket) {
                for (int msg_idx = 0; msg_idx < controller_msg_count; ++msg_idx) {
                    const int write_res = write(dev_in_data->communication.endpoint.socket.fd, (void*)&controller_msg[msg_idx], sizeof(in_message_t));
                    if (write_res < 0) {
                        fprintf(stderr, "Error in writing input event messages: %d -- connection will be drop and retried\n", write_res);

                        // in case of an error reschedule to socket for reconnection
                        close(dev_in_data->communication.endpoint.socket.fd);
                        dev_in_data->communication.endpoint.socket.fd = -1;
                    }
                }
            } else if (dev_in_data->communication.type == ipc_unix_pipe) {
                for (int msg_idx = 0; msg_idx < controller_msg_count; ++msg_idx) {
                    const int write_res = write(dev_in_data->communication.endpoint.pipe.in_message_pipe_fd, (void*)&controller_msg[msg_idx], sizeof(in_message_t));
                    if (write_res < 0) {
                        fprintf(stderr, "Error in writing input event messages: %d\n", write_res);
                    }
                }
            }
        }
    }

    // end communication
    if (dev_in_data->communication.type == ipc_server_sockets) {
        // close every client socket
        if (pthread_mutex_lock(&dev_in_data->communication.endpoint.ssocket.mutex) == 0) {
            for (int i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                close(dev_in_data->communication.endpoint.ssocket.clients[i]);
                dev_in_data->communication.endpoint.ssocket.clients[i] = -1;
            }

            pthread_mutex_unlock(&dev_in_data->communication.endpoint.ssocket.mutex);
        }
    } else if (dev_in_data->communication.type == ipc_unix_pipe) {
        close(dev_in_data->communication.endpoint.pipe.in_message_pipe_fd);
        close(dev_in_data->communication.endpoint.pipe.out_message_pipe_fd);
    } else if (dev_in_data->communication.type == ipc_client_socket) {
        close(dev_in_data->communication.endpoint.socket.fd);
        dev_in_data->communication.endpoint.socket.fd = -1;
    }

    // close every opened device
    for (size_t i = 0; i < max_devices; ++i) {
        if (devices[i].type == DEV_IN_TYPE_EV) {
            evdev_close_device(&devices[i].dev.evdev);
            devices[i].type = DEV_IN_TYPE_NONE;
        } else if (devices[i].type == DEV_IN_TYPE_IIO) {
            iio_close_device(&devices[i].dev.iio);
            devices[i].type = DEV_IN_TYPE_NONE;
        } else if (devices[i].type == DEV_IN_TYPE_HIDRAW) {
            hidraw_close_device(&devices[i].dev.hidraw);
            devices[i].type = DEV_IN_TYPE_NONE;
        }
    }

    free(devices);

    if (platform_init_res != 0) {
        dev_in_data->input_dev_decl->deinit_fn(&dev_in_data->settings, &platform_data);
    }

    return NULL;
}
