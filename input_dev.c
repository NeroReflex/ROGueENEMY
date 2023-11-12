#include "input_dev.h"
#include "message.h"
#include "queue.h"
#include "dev_iio.h"
#include "platform.h"

#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <dirent.h> 
#include <stdio.h> 

static const char *input_path = "/dev/input/";
static const char *iio_path = "/sys/bus/iio/devices/";

int input_filter_identity(struct input_event* events, size_t* size, uint32_t* count) {
    return INPUT_FILTER_RESULT_OK;
}

int input_filter_asus_kb(struct input_event* events, size_t* size, uint32_t* count) {
    static int F15_status = 0;

    if ((*count >= 2) && (events[0].type == EV_MSC) && (events[0].code == MSC_SCAN)) {
        if ((events[0].value == -13565784) && (events[1].type == EV_KEY) && (events[1].code == KEY_F18)) {
            if (events[1].value == 1) {
                printf("Detected mode switch command, switching mode...\n");
                cycle_mode();
            } else {
                // Do nothing effectively discarding the input
            }

            return INPUT_FILTER_RESULT_DO_NOT_EMIT;
        } else if (events[0].value == -13565784) {
            return INPUT_FILTER_RESULT_DO_NOT_EMIT;
        } else if ((*count == 2) && (events[0].value == 458860) && (events[1].type == EV_KEY) && (events[1].code == KEY_F17)) {
            if (events[1].value < 2) {
                *count = 1;
                events[0].type = EV_KEY;
                events[0].code = BTN_GEAR_DOWN;
                events[0].value = events[1].value;
                return INPUT_FILTER_RESULT_OK;
            }

            return INPUT_FILTER_RESULT_DO_NOT_EMIT;
        } else if ((*count == 2) && (events[0].value == 458861) && (events[1].type == EV_KEY) && (events[1].code == KEY_F18)) {
            if (events[1].value < 2) {
                *count = 1;
                events[0].type = EV_KEY;
                events[0].code = BTN_GEAR_UP;
                events[0].value = events[1].value;
                return INPUT_FILTER_RESULT_OK;
            }

            return INPUT_FILTER_RESULT_DO_NOT_EMIT;
        } else if ((*count == 2) && (events[0].value == -13565786) && (events[1].type == EV_KEY) && (events[1].code == KEY_F16)) {
            *count = 1;
            events[0].type = EV_KEY;
            events[0].code = BTN_MODE;
            events[0].value = events[1].value;

            return INPUT_FILTER_RESULT_OK;
        } else if ((*count == 2) && (events[0].value == -13565787) && (events[1].type == EV_KEY) && (events[1].code == KEY_F15)) {
            if (events[1].value == 0) {
                if (F15_status > 0) {
                    --F15_status;
                }

                if (F15_status == 0) {
                    printf("Exiting gyro mode.\n");
                }
            } else if (events[1].value == 1) {
                if (F15_status <= 2) {
                    ++F15_status;
                }

                if (F15_status == 1) {
                    printf("Entering gyro mode.\n");
                }
            }

            return INPUT_FILTER_RESULT_DO_NOT_EMIT;
        } else if ((*count == 2) && (*size >= 3) && (events[0].value == -13565896) && (events[1].type == EV_KEY) && (events[1].code == KEY_PROG1)) {
            *count = 3;

            int32_t val = events[1].value;
            struct timeval time = events[1].time;

            events[0].type = EV_KEY;
            events[0].code = BTN_MODE;
            events[0].value = val;

            events[1].type = SYN_REPORT;
            events[1].code = EV_SYN;
            events[1].value = 0;

            events[2].type = EV_KEY;
            events[2].code = BTN_SOUTH;
            events[2].value = val;
/*
            events[3].type = SYN_REPORT;
            events[3].code = EV_SYN;
            events[3].value = 0;

            events[4].type = EV_KEY;
            events[4].code = BTN_SOUTH;
            events[4].value = 0;
*/
            return INPUT_FILTER_RESULT_OK;
        }
    }

    return INPUT_FILTER_RESULT_OK;
}

static struct libevdev* ev_matches(const char* sysfs_entry, const uinput_filters_t* const filters) {
    struct libevdev *dev = NULL;

    int fd = open(sysfs_entry, O_RDWR);
    if (fd < 0) {
        //fprintf(stderr, "Cannot open %s, device skipped.\n", sysfs_entry);
        return NULL;
    }

    if (libevdev_new_from_fd(fd, &dev) != 0) {
        //fprintf(stderr, "Cannot initialize libevdev from this device (%s): skipping.\n", sysfs_entry);
        close(fd);
        return NULL;
    }

    const char* name = libevdev_get_name(dev);
    if ((name != NULL) && (strcmp(name, filters->name) != 0)) {
        //fprintf(stderr, "The device name (%s) for device %s does not matches the expected one %s.\n", name, sysfs_entry, filters->name);
        libevdev_free(dev);
        close(fd);
        return NULL;
    }

    const int grab_res = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (grab_res != 0) {
        fprintf(stderr, "Unable to grab the device (%s): %d.\n", sysfs_entry, grab_res);
        //libevdev_free(dev);
        //close(fd);
        return dev;
    }

    return dev;
}

static dev_iio_t* iio_matches(const char* sysfs_entry, const iio_filters_t* const filters) {
    dev_iio_t *const dev_iio = dev_iio_create(sysfs_entry);
    if (dev_iio == NULL) {
        fprintf(stderr, "Could not create iio device.\n");
        return NULL;
    }

    const char* const iio_name = dev_iio_get_name(dev_iio);
    if (abs(strcmp(iio_name, filters->name)) != 0) {
        fprintf(stderr, "Error: iio device name does not match, expected %s got %s.\n", filters->name, iio_name);
        dev_iio_destroy(dev_iio);
        return NULL;
    }

    return dev_iio;
}

static pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* open_sysfs[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

#define MAX_MESSAGES_IN_FLIGHT 32
#define DEFAULT_EVENTS_IN_REPORT 8


struct input_ctx {
    struct libevdev* dev;
    dev_iio_t *iio_dev;
    queue_t* queue;
    message_t messages[MAX_MESSAGES_IN_FLIGHT];
    input_filter_t input_filter_fn;
};

static void* iio_read_thread_func(void* ptr) {
    struct input_ctx* ctx = (struct input_ctx*)ptr;

    message_t* msg = NULL;
    
    int rc = -1;

    do {
        if (msg == NULL) {
            for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
                if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE) != 0) {
                    msg = &ctx->messages[h];
                    msg->ev_count = 0;
                    break;
                }
            }
        }

        if (msg == NULL) {
            fprintf(stderr, "iio: Events are stalled.\n");
            continue;
        }

        rc = dev_iio_read(ctx->iio_dev, msg->ev, msg->ev_size, &msg->ev_count);
        if (rc == 0) {
            // OK: good read. go on....
        } else if (rc == -ENOMEM) {
            fprintf(stderr, "Error: out-of-memory will skip the current frame.\n");
            continue;
        } else {
            fprintf(stderr, "Error: reading %s: %d\n", dev_iio_get_name(ctx->iio_dev), rc);
            break;
        }

        // clear out flags
        msg->flags = 0x00000000U;

        if (queue_push(ctx->queue, (void*)msg) != 0) {
            fprintf(stderr, "Error pushing event.\n");

            // flag the memory to be safe to reuse
            msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
        }

        usleep(100);

        // either way.... fill a new buffer on the next cycle
        msg = NULL;
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    return NULL;
}

static void* input_read_thread_func(void* ptr) {
    struct input_ctx* ctx = (struct input_ctx*)ptr;
    struct libevdev* dev = ctx->dev;

    int has_syn = libevdev_has_event_type(ctx->dev, EV_SYN);

    int rc = 1;

    message_t* msg = NULL;
    
    do {
        if (msg == NULL) {
            for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
                if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE) != 0) {
                    msg = &ctx->messages[h];
                    msg->ev_count = 0;
                    break;
                }
            }
        }

        if (msg == NULL) {
            fprintf(stderr, "udev: Events are stalled.\n");
            continue;
        }

        struct input_event read_ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &read_ev);
        if (rc == 0) {
            const int is_syn = (read_ev.type == EV_SYN) && (read_ev.code == SYN_REPORT);
            
            if (read_ev.type == EV_MSC) {
                if (read_ev.code == MSC_SCAN) {
#if defined(IGNORE_INPUT_SCAN)
                    continue;
#endif // IGNORE_INPUT_SCAN
                } else if (read_ev.code == MSC_TIMESTAMP) {
                    // the output device will handle that
                    //printf("MSC_TIMESTAMP found. Ignoring...\n");
                }
            }

            if ((!has_syn) || ((has_syn) && (!is_syn))) {
                if ((msg->ev_count+1) == msg->ev_size) {
                    // TODO: perform a memove
                    fprintf(stderr, "MEMMOVE NEEDED\n");
                } else {
#if defined(INCLUDE_INPUT_DEBUG)
                    printf(
                        "Input: %s %s %d\n",
                        libevdev_event_type_get_name(read_ev.type),
                        libevdev_event_code_get_name(read_ev.type, read_ev.code),
                        read_ev.value
                    );
#endif

                    // just copy the input event
                    msg->ev[msg->ev_count] = read_ev;
                    ++msg->ev_count;
                }
            }

            if ((!has_syn) || ((has_syn) && (is_syn))) {
                /*
                printf("Sync ---------------------------------------\n");
                */

                // clear out flags
                msg->flags = 0x00000000U;

                if (ctx->input_filter_fn(msg->ev, &msg->ev_size, &msg->ev_count) != INPUT_FILTER_RESULT_DO_NOT_EMIT) {
                    if (queue_push(ctx->queue, (void*)msg) != 0) {
                        fprintf(stderr, "Error pushing event.\n");

                        // flag the memory to be safe to reuse
                        msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
                    }
                } else {
                    // flag the memory to be safe to reuse
                    msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
                }

                // either way.... fill a new buffer on the next cycle
                msg = NULL;
            }

            
        }
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    return NULL;
}

static void input_iio(
    input_dev_t *const in_dev,
    struct input_ctx *const ctx
) {
    int open_sysfs_idx = -1;

    for (;;) {
        const uint32_t flags = in_dev->crtl_flags;
        if (flags & INPUT_DEV_CTRL_FLAG_EXIT) {
            in_dev->crtl_flags &= ~INPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }

        // clean up from previous iteration
        if (ctx->iio_dev != NULL) {
            dev_iio_destroy(ctx->iio_dev);
            ctx->dev = NULL;
        }
        
        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            fprintf(stderr, "Cannot lock input mutex: %d, will retry later...\n", input_acquire_lock_result);
            usleep(250000);
            continue;
        }

        // clean up leftover from previous opening
        if (open_sysfs_idx >= 0) {
            free(open_sysfs[open_sysfs_idx]);
            open_sysfs[open_sysfs_idx] = NULL;
        }

        char path[512] = "\0";
        
        DIR *d;
        struct dirent *dir;
        d = opendir(iio_path);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.') {
                    continue;
                } else if (dir->d_name[0] == 'b') { // by-id
                    continue;
                } else if (dir->d_name[0] == 'j') { // js-0
                    continue;
                }

                sprintf(path, "%s%s", iio_path, dir->d_name);

                // check if that has been already opened
                // open_sysfs
                int skip = 0;
                for (int o = 0; o < (sizeof(open_sysfs) / sizeof(const char*)); ++o) {
                    if ((open_sysfs[o] != NULL) && (strcmp(open_sysfs[o], path) == 0)) {
                        fprintf(stderr, "already opened iio device %s: skip.\n", path);
                        skip = 1;
                        break;
                    }
                }

                if (skip) {
                    continue;
                }

                // try to open the device
                ctx->iio_dev = iio_matches(path, in_dev->iio_filters);
                if (ctx->iio_dev != NULL) {
                    open_sysfs_idx = 0;
                    while (open_sysfs[open_sysfs_idx] != NULL) {
                        ++open_sysfs_idx;
                    }
                    open_sysfs[open_sysfs_idx] = malloc(sizeof(path));
                    memcpy(open_sysfs[open_sysfs_idx], path, 512);    

                    printf("Opened iio %s\n    name: %s\n",
                        path,
                        dev_iio_get_name(ctx->iio_dev)
                    );
                    
                    break;
                } else {
                    fprintf(stderr, "iio device in %s does NOT matches\n", path);
                    ctx->iio_dev = NULL;
                }
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        // TODO: if device was not open "continue"
        if (ctx->iio_dev == NULL) {
            usleep(250000);
            continue;
        }

        pthread_t incoming_events_thread;

        const int incoming_events_thread_creation = pthread_create(&incoming_events_thread, NULL, iio_read_thread_func, (void*)ctx);
        if (incoming_events_thread_creation != 0) {
            fprintf(stderr, "Error creating the input thread for device %s: %d\n", dev_iio_get_name(ctx->iio_dev), incoming_events_thread_creation);
        }

        if (incoming_events_thread_creation == 0) {
            pthread_join(incoming_events_thread, NULL);
        }
    }
}

static void input_udev(
    input_dev_t *const in_dev,
    struct input_ctx *const ctx
) {
    int open_sysfs_idx = -1;

    for (;;) {
        const uint32_t flags = in_dev->crtl_flags;
        if (flags & INPUT_DEV_CTRL_FLAG_EXIT) {
            in_dev->crtl_flags &= ~INPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }

        // clean up from previous iteration
        if (ctx->dev != NULL) {
            libevdev_free(ctx->dev);
            ctx->dev = NULL;
        }

        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            fprintf(stderr, "Cannot lock input mutex: %d, will retry later...\n", input_acquire_lock_result);
            usleep(250000);
            continue;
        }

        // clean up leftover from previous opening
        if (open_sysfs_idx >= 0) {
            free(open_sysfs[open_sysfs_idx]);
            open_sysfs[open_sysfs_idx] = NULL;
        }

        char path[512] = "\0";
        
        DIR *d;
        struct dirent *dir;
        d = opendir(input_path);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.') {
                    continue;
                } else if (dir->d_name[0] == 'b') { // by-id
                    continue;
                } else if (dir->d_name[0] == 'j') { // js-0
                    continue;
                }

                sprintf(path, "%s%s", input_path, dir->d_name);

                // check if that has been already opened
                // open_sysfs
                int skip = 0;
                for (int o = 0; o < (sizeof(open_sysfs) / sizeof(const char*)); ++o) {
                    if ((open_sysfs[o] != NULL) && (strcmp(open_sysfs[o], path) == 0)) {
                        skip = 1;
                        break;
                    }
                }

                if (skip) {
                    continue;
                }

                // try to open the device
                ctx->dev = ev_matches(path, in_dev->ev_filters);
                if (ctx->dev != NULL) {
                    open_sysfs_idx = 0;
                    while (open_sysfs[open_sysfs_idx] != NULL) {
                        ++open_sysfs_idx;
                    }
                    open_sysfs[open_sysfs_idx] = malloc(sizeof(path));
                    memcpy(open_sysfs[open_sysfs_idx], path, 512);    

                    if (libevdev_has_event_type(ctx->dev, EV_FF)) {
                        printf("Opened device %s\n    name: %s\n    rumble: %s\n",
                            path,
                            libevdev_get_name(ctx->dev),
                            libevdev_has_event_code(ctx->dev, EV_FF, FF_RUMBLE) ? "true" : "false"
                        );
                    } else {
                        printf("Opened device %s\n    name: %s\n    rumble: no EV_FF\n",
                            path,
                            libevdev_get_name(ctx->dev)
                        );
                    }
                    
                    break;
                }
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        if (ctx->dev == NULL) {
            usleep(250000);
            continue;
        }

        pthread_t incoming_events_thread;

        const int incoming_events_thread_creation = pthread_create(&incoming_events_thread, NULL, input_read_thread_func, (void*)ctx);
        if (incoming_events_thread_creation != 0) {
            fprintf(stderr, "Error creating the input thread for device %s: %d\n", libevdev_get_name(ctx->dev), incoming_events_thread_creation);
        }

        if (incoming_events_thread_creation == 0) {
            pthread_join(incoming_events_thread, NULL);
        }
    }
}

void *input_dev_thread_func(void *ptr) {
    input_dev_t *in_dev = (input_dev_t*)ptr;

    struct input_ctx ctx = {
        .dev = NULL,
        .queue = in_dev->queue,
        .input_filter_fn = in_dev->input_filter_fn,
    };

    for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
        ctx.messages[h].flags = MESSAGE_FLAGS_HANDLE_DONE;
        ctx.messages[h].ev_size = DEFAULT_EVENTS_IN_REPORT;
        ctx.messages[h].ev = malloc(sizeof(struct input_event) * ctx.messages[h].ev_size);
    }

    if (in_dev->dev_type == input_dev_type_uinput) {
        input_udev(in_dev, &ctx);
    } else if (in_dev->dev_type == input_dev_type_iio) {
        input_iio(in_dev, &ctx);
    }
    
    return NULL;
}
