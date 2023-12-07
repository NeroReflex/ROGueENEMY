#include "dev_evdev.h"
#include <libevdev-1.0/libevdev/libevdev.h>

static const char *input_path = "/dev/input/";

static pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

static int open_fds[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
};

static bool ev_matches(struct libevdev *dev, const uinput_filters_t* const filters) {

    const char* name = libevdev_get_name(dev);
    if ((name != NULL) && (strcmp(name, filters->name) != 0)) {
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
    for (int i = 0; i < sizeof(open_fds) / sizeof(int); ++i) {
        open_fds[i] = (open_fds[i] == fd) ? -1 : open_fds[i];
    }

    // free the memory
    libevdev_free(out_evdev);

    pthread_mutex_unlock(&input_acquire_mutex);
}

#define MAX_PATH_LEN 512

int dev_evdev_open(
    const uinput_filters_t *const in_filters,
    struct libevdev* out_evdev
) {
    int res = -ENOENT;

    int open_sysfs_idx = -1;

    const int mutex_lock_res = pthread_mutex_lock(&input_acquire_mutex);
    if (mutex_lock_res != 0) {
        fprintf(stderr, "Cannot lock input mutex: %d\n", mutex_lock_res);
        res = mutex_lock_res;
        goto dev_evdev_open_err;
    }

    char path[MAX_PATH_LEN] = "\0";
    
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

            // try to open the device, if it cannot be opened to go the next
            int fd = open(path, O_RDWR);
            if (fd < 0) {
                //fprintf(stderr, "Cannot open %s, device skipped.\n", sysfs_entry);
                continue;
            }

            // check if that has been already opened
            // open_sysfs
            int skip = 0;
            for (int o = 0; o < (sizeof(open_fds) / sizeof(open_fds[0])); ++o) {
                if ((open_fds[o] != -1) && (open_fds[o] == fd)) {
                    close(fd);
                    skip = 1;
                    break;
                } else if (open_fds[o] == -1) {
                    open_sysfs_idx = o;
                }
            }

            if ((skip) || (open_sysfs_idx == -1)) {
                continue;
            } else {
                open_fds[open_sysfs_idx] = fd;
            }

            if (libevdev_new_from_fd(fd, &out_evdev) != 0) {
                fprintf(stderr, "Cannot initialize libevdev from this device (%s) -- Skipping.\n", path);
                close(fd);
                continue;
            } else {
                printf("Acquired evdev device %s: fd=%d", path, fd);
            }

            // try to open the device
            if (!ev_matches(out_evdev, in_filters)) {
                libevdev_free(out_evdev);
                close(fd);
                continue;
            }

            // the device has been found
            break;
        }
        closedir(d);
    }

dev_evdev_open_err:
    pthread_mutex_unlock(&input_acquire_mutex);

    return res;
}
