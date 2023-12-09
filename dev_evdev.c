#include "dev_evdev.h"
#include <libevdev-1.0/libevdev/libevdev.h>

static const char *input_path = "/dev/input/";

static pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

struct open_resource {
    char* sysfs_path;
    int fd;
};

static struct open_resource open_paths[] = {
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
    {
        .fd = -1,
        .sysfs_path = NULL
    },
};

static bool ev_matches(
    const uinput_filters_t* const in_filters,
    struct libevdev *in_evdev
) {
    if (in_evdev == NULL) {
        return NULL;
    }

    const char* name = libevdev_get_name(in_evdev);
    if ((name != NULL) && (strcmp(name, in_filters->name) != 0)) {
        return false;
    }

    // TODO: if more filters are implemented write them here

    return true;
}

void dev_evdev_close(struct libevdev* out_evdev) {
    if (out_evdev == NULL) {
        return;
    }

    const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
    if (input_acquire_lock_result != 0) {
        fprintf(stderr, "Cannot lock input mutex: %d -- this will probably have no consequences\n", input_acquire_lock_result);
        return;
    }

    int fd = libevdev_get_fd(out_evdev);
    for (int i = 0; i < sizeof(open_paths) / sizeof(open_paths[0]); ++i) {
        if (open_paths[i].fd == fd) {
            if (open_paths[i].sysfs_path != NULL) {
                free(open_paths[i].sysfs_path);
                open_paths[i].sysfs_path = NULL;
            }
            open_paths[i].fd = -1;
        }
    }

    // free the memory
    libevdev_free(out_evdev);

    pthread_mutex_unlock(&input_acquire_mutex);
}

#define MAX_PATH_LEN 512

int dev_evdev_open(
    const uinput_filters_t *const in_filters,
    struct libevdev* *const out_evdev
) {
    int res = -ENOENT;

    int open_sysfs_idx = -1;

    const int mutex_lock_res = pthread_mutex_lock(&input_acquire_mutex);
    if (mutex_lock_res != 0) {
        fprintf(stderr, "Cannot lock input mutex: %d\n", mutex_lock_res);
        res = mutex_lock_res;
        goto dev_evdev_open_mutex_err;
    }

    char *const path = malloc(MAX_PATH_LEN);
    if (path == NULL) {
        res = -ENOMEM;
        goto dev_evdev_open_err;
    }
    
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

            snprintf(path, MAX_PATH_LEN - 1, "%s%s", input_path, dir->d_name);

            //printf("Testing for device %s\n", path);

            // try to open the device, if it cannot be opened to go the next
            int fd = open(path, O_RDWR);
            if (fd < 0) {
                fprintf(stderr, "Cannot open %s, device skipped.\n", path);
                continue;
            }

            // check if that has been already opened
            // open_sysfs
            int skip = 0;
            for (int o = 0; o < (sizeof(open_paths) / sizeof(open_paths[0])); ++o) {
                if ((open_paths[o].fd != -1) && (open_paths[o].sysfs_path != NULL) && (strcmp(open_paths[o].sysfs_path, path) == 0)) {
                    close(fd);
                    skip = 1;
                    break;
                } else if ((open_paths[o].sysfs_path == NULL) && (open_paths[o].fd == -1)) {
                    open_sysfs_idx = o;
                }
            }

            if ((skip) || (open_sysfs_idx == -1)) {
                free(path);
                continue;
            }

            if (libevdev_new_from_fd(fd, out_evdev) != 0) {
                free(path);
                close(fd);
                continue;
            }

            // try to open the device
            if (!ev_matches(in_filters, *out_evdev)) {
                libevdev_free(*out_evdev);
                free(path);
                close(fd);
                continue;
            }

            // register the device as being opened already
            open_paths[open_sysfs_idx].sysfs_path = path;
            open_paths[open_sysfs_idx].fd = fd;

            // the device has been found
            res = 0;
            break;
        }
        closedir(d);
    }

dev_evdev_open_err:
    if ((path != NULL) && (res != 0)) {
        free(path);
    }

    pthread_mutex_unlock(&input_acquire_mutex);

dev_evdev_open_mutex_err:
    return res;
}
