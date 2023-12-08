#include "dev_in.h"
#include "message.h"
#include "dev_evdev.h"
#include <libevdev-1.0/libevdev/libevdev.h>

typedef enum dev_in_type {
    DEV_IN_TYPE_NONE,
    DEV_IN_TYPE_HIDRAW,
    DEV_IN_TYPE_IIO,
    DEV_IN_TYPE_EV,
} dev_in_type_t;

typedef struct dev_in_iio {

    int fd;

} dev_in_iio_t;

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
} dev_in_aggr_t;

typedef struct dev_in {

    dev_in_type_t type;

    dev_in_aggr_t dev;

} dev_in_t;

static int fill_message_from_iio(dev_in_iio_t *const in_evdev, in_message_t *const out_msg) {
    return -EINVAL;
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

int open_device(
    const uinput_filters_t *const in_filters,
    dev_in_ev_t *const out_dev
) {
    int res = dev_evdev_open(in_filters, &out_dev->evdev);
    if (res != 0) {
        fprintf(stderr, "Unable to open the specified device: %d\n", res);
        goto open_device_err;
    } else {
        printf("Device opened: %s\n", libevdev_get_name(out_dev->evdev));
    }

    out_dev->has_rumble_support = libevdev_has_event_type(out_dev->evdev, EV_FF) && libevdev_has_event_code(out_dev->evdev, EV_FF, FF_RUMBLE);

    const int grab_res = libevdev_grab(out_dev->evdev, LIBEVDEV_GRAB);
    out_dev->grabbed = grab_res == 0;
    if (!out_dev->grabbed) {
        fprintf(stderr, "Unable to grab the device (%s): %d.\n", libevdev_get_name(out_dev->evdev), grab_res);
    }

    if (out_dev->has_rumble_support) {
        printf("Opened device\n    name: %s\n    rumble: %s\n",
            libevdev_get_name(out_dev->evdev),
            libevdev_has_event_code(out_dev->evdev, EV_FF, FF_RUMBLE) ? "true" : "false"
        );

        // prepare the rumble effect
        out_dev->ff_effect.type = FF_RUMBLE;
        out_dev->ff_effect.id = -1;
        out_dev->ff_effect.replay.delay = 0;
        out_dev->ff_effect.replay.length = 5000;
        out_dev->ff_effect.u.rumble.strong_magnitude = 0x0000;
        out_dev->ff_effect.u.rumble.weak_magnitude = 0x0000;

        const struct input_event gain = {
            .type = EV_FF,
            .code = FF_GAIN,
            .value = 0xFFFF, // TODO: give the user the ability to change this
        };

        const int gain_set_res = write(libevdev_get_fd(out_dev->evdev), (const void*)&gain, sizeof(gain));
        if (gain_set_res != sizeof(gain)) {
            fprintf(stderr, "Unable to adjust gain for force-feedback: %d\n", gain_set_res);
        } else {
            printf("Gain for force-feedback set to %u for device %s\n", gain.value, libevdev_get_name(out_dev->evdev));
        }

    } else {
        printf("Opened device\n    name: %s\n    rumble: no force-feedback\n",
            libevdev_get_name(out_dev->evdev)
        );
    }

open_device_err:
    return res;
}

static void handle_rumble_device(dev_in_ev_t *const in_dev, const out_message_rumble_t *const in_rumble_msg) {
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
    in_dev->ff_effect.u.rumble.strong_magnitude = in_rumble_msg->strong_magnitude;
    in_dev->ff_effect.u.rumble.weak_magnitude = in_rumble_msg->weak_magnitude;

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

static void handle_rumble(dev_in_t *const in_devs, size_t in_devs_count, const out_message_rumble_t *const in_rumble_msg) {
    for (size_t i = 0; i < in_devs_count; ++i) {
        if (in_devs[i].type == DEV_IN_TYPE_EV) {
            handle_rumble_device(&in_devs[i].dev.evdev, in_rumble_msg);
        }
    }
}

void* dev_in_thread_func(void *ptr) {
    dev_in_data_t *const devs = (dev_in_data_t*)ptr;

    struct timeval timeout = {
        .tv_sec = (__time_t)devs->timeout_ms / (__time_t)1000,
        .tv_usec = ((__suseconds_t)devs->timeout_ms % (__suseconds_t)1000) * (__suseconds_t)1000000,
    };

    fd_set read_fds;
    
    dev_in_t* const devices = malloc(sizeof(dev_in_t) * devs->input_dev_cnt);
    if (devices == NULL) {
        fprintf(stderr, "Unable to allocate memory to hold devices -- aborting input thread\n");
        return NULL;
    }

    // flag every device as disconnected
    for (size_t i = 0; i < devs->input_dev_cnt; ++i) {
        devices[i].type = DEV_IN_TYPE_NONE;
    }

    for (;;) {
        

        FD_ZERO(&read_fds);
        FD_SET(devs->in_message_pipe_fd, &read_fds);
        for (size_t i = 0; i < devs->input_dev_cnt; ++i) {
            int fd = -1;
            if (devices[i].type == DEV_IN_TYPE_EV) {
                fd = libevdev_get_fd(devices[i].dev.evdev.evdev);

                // device is present, query it in select
                FD_SET(fd, &read_fds);
            } else if (devices[i].type == DEV_IN_TYPE_IIO) {

            } else if (devices[i].type == DEV_IN_TYPE_NONE) {
                fprintf(stderr, "Device %zu not found -- Attempt reconnection\n", i);

                if (devs->input_dev_decl[i].dev_type == input_dev_type_uinput) {
                    const int open_res = open_device(&devs->input_dev_decl[i].filters.ev, &devices[i].dev.evdev);
                    if (open_res == 0) {
                        devices[i].type = DEV_IN_TYPE_EV;
                        fd = libevdev_get_fd(devices[i].dev.evdev.evdev);

                        // device is now connected, query it in select
                        FD_SET(fd, &read_fds);
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
        if (FD_ISSET(devs->out_message_pipe_fd, &read_fds)) {
            out_message_t out_msg;
            const ssize_t out_message_pipe_read_res = read(devs->out_message_pipe_fd, (void*)&out_msg, sizeof(out_message_t));
            if (out_message_pipe_read_res == sizeof(out_message_t)) {
                if (out_msg.type == OUT_MSG_TYPE_RUMBLE) {
                    handle_rumble(devices, devs->input_dev_cnt, &out_msg.data.rumble);
                } else if (out_msg.type == OUT_MSG_TYPE_LEDS) {
                    // TODO: handle LEDs
                }
            } else {
                fprintf(stderr, "Unable to read message: %zd\n", out_message_pipe_read_res);
            }
        }

        // the following is only executed when there is actual data in at least one of the fd
        for (size_t i = 0; i < devs->input_dev_cnt; ++i) {
            int fd = -1;
            if (devices[i].type == DEV_IN_TYPE_EV) {
                fd = libevdev_get_fd(devices[i].dev.evdev.evdev);
            } else if (devices[i].type == DEV_IN_TYPE_IIO) {
                // TODO: implement IIO
            }
            
            if (!FD_ISSET(fd, &read_fds)) {
                continue;
            }

            if (devices[i].type == DEV_IN_TYPE_EV) {
                evdev_collected_t coll = {
                    .ev_count = 0
                };

                const int fill_res = fill_message_from_evdev(&devices[i].dev.evdev, &coll);
                if (fill_res != 0) {
                    fprintf(stderr, "Unable to fill input_event(s) for device %zd: %d\n", i, fill_res);
                    continue;
                } else {
                    devs->input_dev_decl[i].ev_input_map_fn(&coll, devs->in_message_pipe_fd, devs->input_dev_decl[i].user_data);
                }
            } else if (devices[i].type == DEV_IN_TYPE_EV) {
                // TODO: implement IIO
                //fill_message_from_iio(&devices[i].dev.iio, out_msg);
            }
        }
    }

    // TODO: free every fd
    free(devices);

    return NULL;
}
