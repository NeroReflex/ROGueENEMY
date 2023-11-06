#include "input_dev.h"
#include "message.h"
#include "queue.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <dirent.h> 
#include <stdio.h> 

static const char *input_path = "/dev/input/";

static struct libevdev* ev_matches(const char* sysfs_entry, const uinput_filters_t* const filters) {
    struct libevdev *dev = NULL;

    int fd = open(sysfs_entry, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s, device skipped.\n", sysfs_entry);
        return NULL;
    }

    if (libevdev_new_from_fd(fd, &dev) != 0) {
        fprintf(stderr, "Cannot initialize libevdev from this device (%s): skipping.\n", sysfs_entry);
        close(fd);
        return NULL;
    }

    const char* name = libevdev_get_name(dev);
    if ((name != NULL) && (strcmp(name, filters->name) != 0)) {
        fprintf(stderr, "The device name (%s) for device %s does not matches the expected one %s.\n", name, sysfs_entry, filters->name);
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

static pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* open_sysfs[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

#define MAX_MESSAGES_IN_FLIGHT 32

struct input_ctx {
    struct libevdev* dev;
    queue_t* queue;
    message_t messages[MAX_MESSAGES_IN_FLIGHT];
};

void* input_read_thread_func(void* ptr) {
    struct input_ctx* ctx = (struct input_ctx*)ptr;
    struct libevdev* dev = ctx->dev;

    int rc = 1;

    do {
        message_t* msg = NULL;
        for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
            if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE) != 0) {
                msg = &ctx->messages[h];

                // clear out flags
                msg->flags = 0x00000000U;
                
                // search is over
                break;
            }
        }

        if (msg == NULL) {
            fprintf(stderr, "Events are stalled.\n");
            continue;
        }

        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &msg->ev);
        if (rc == 0) {
            if (queue_push(ctx->queue, (void*)msg) != 0) {
                fprintf(stderr, "Error pushing event.\n");

                // flag the memory to be safe to reuse
                msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
                continue;
            }
        }
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    return NULL;
}

void *input_dev_thread_func(void *ptr) {
    input_dev_t *in_dev = (input_dev_t*)ptr;

    struct input_ctx ctx = {
        .dev = NULL,
        .queue = in_dev->queue,
        .messages = {}
    };

    for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
        ctx.messages[h].flags = MESSAGE_FLAGS_HANDLE_DONE;
    }

    int open_sysfs_idx = -1;

    for (;;) {
        const uint32_t flags = in_dev->crtl_flags;
        if (flags & INPUT_DEV_CTRL_FLAG_EXIT) {
            in_dev->crtl_flags &= ~INPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }

        // clean up from previous iteration
        if (ctx.dev != NULL) {
            libevdev_free(ctx.dev);
            ctx.dev = NULL;
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
                ctx.dev = ev_matches(path, in_dev->ev_filters);
                if (ctx.dev != NULL) {
                    open_sysfs_idx = 0;
                    while (open_sysfs[open_sysfs_idx] != NULL) {
                        ++open_sysfs_idx;
                    }
                    open_sysfs[open_sysfs_idx] = malloc(sizeof(path));
                    memcpy(open_sysfs[open_sysfs_idx], path, 512);    

                    if (libevdev_has_event_type(ctx.dev, EV_FF)) {
                        printf("Opened device %s\n    name: %s\n    rumble: %s\n",
                            path,
                            libevdev_get_name(ctx.dev),
                            libevdev_has_event_code(ctx.dev, EV_FF, FF_RUMBLE) ? "true" : "false"
                        );
                    } else {
                        printf("Opened device %s\n    name: %s\n    rumble: no EV_FF\n",
                            path,
                            libevdev_get_name(ctx.dev)
                        );
                    }
                    
                    break;
                }
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        if (ctx.dev == NULL) {
            usleep(250000);
            continue;
        }

        pthread_t incoming_events_thread;

        const int incoming_events_thread_creation = pthread_create(&incoming_events_thread, NULL, input_read_thread_func, (void*)&ctx);
        if (incoming_events_thread_creation != 0) {
            fprintf(stderr, "Error creating the input thread for device %s: %d\n", libevdev_get_name(ctx.dev), incoming_events_thread_creation);
        }

        if (incoming_events_thread_creation == 0) {
            pthread_join(incoming_events_thread, NULL);
        }
    }

    return NULL;
}
