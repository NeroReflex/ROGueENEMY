#include "dev_hidraw.h"

#define MAX_PATH_LEN 512

static const char *const hidraw_path = "/dev/";

static bool hidraw_matches(
    const hidraw_filters_t *const in_filters,
    dev_hidraw_t *const in_dev
) {
    if (in_dev->info.product != in_filters->pid) {
        return false;
    } else if (in_dev->info.vendor != in_filters->vid) {
        return false;
    } else if (in_dev->rdesc.size != in_filters->rdesc_size) {
        return false;
    }

    return true;
}

static int dev_hidraw_new_from_fd(int fd, dev_hidraw_t **const out_dev) {
    int res = -EINVAL;

    *out_dev = malloc(sizeof(dev_hidraw_t));
    if (out_dev == NULL) {
        fprintf(stderr, "Cannot allocate memory for hidraw device\n");
        res = -ENOMEM;
        goto dev_hidraw_new_from_fd_err;
    }

    (*out_dev)->fd = fd;
    memset(&(*out_dev)->info, 0, sizeof(struct hidraw_devinfo));
    memset(&(*out_dev)->rdesc, 0, sizeof(struct hidraw_report_descriptor));

    res = ioctl((*out_dev)->fd, HIDIOCGRAWINFO, &(*out_dev)->info);
    if (res < 0) {
        fprintf(stderr, "Error getting device info: %d\n", errno);
        goto dev_hidraw_new_from_fd_err;
    }

    res = ioctl((*out_dev)->fd, HIDIOCGRDESC, &(*out_dev)->rdesc);
    if (res < 0) {
        perror("Error getting Report Descriptor");
        goto dev_hidraw_new_from_fd_err;
    }

    res = ioctl((*out_dev)->fd, HIDIOCGRDESCSIZE, &(*out_dev)->rdesc.size);
    if (res < 0) {
        perror("Error getting Report Descriptor");
        goto dev_hidraw_new_from_fd_err;
    } else {
        printf("GOT HIDIOCGRDESC %ld\n",(long) (*out_dev)->rdesc.size);
    }

dev_hidraw_new_from_fd_err:
    if (res < 0) {
        free(*out_dev);
        *out_dev = NULL;
    }

    return res;
}

int dev_hidraw_open(
    const hidraw_filters_t *const in_filters,
    dev_hidraw_t **const out_dev
) {
    int res = -ENOENT;

    char path[MAX_PATH_LEN] = "\0";
    
    DIR *d;
    struct dirent *dir;
    d = opendir(hidraw_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, "hidraw") == NULL) {
                continue;
            }

            snprintf(path, MAX_PATH_LEN - 1, "%s%s", hidraw_path, dir->d_name);

            //printf("Testing for device %s\n", path);

            // try to open the device, if it cannot be opened to go the next
            int fd = open(path, O_RDWR);
            if (fd < 0) {
                //fprintf(stderr, "Cannot open %s, device skipped.\n", path);
                continue;
            }

            const int hidraw_open_res = dev_hidraw_new_from_fd(fd, out_dev);
            if (hidraw_open_res != 0) {
                close(fd);
                continue;
            }

            if (!hidraw_matches(in_filters, *out_dev)) {
                dev_hidraw_close(*out_dev);
                continue;
            }

            // the device has been found
            res = 0;
            break;
        }
        closedir(d);
    }

dev_hidraw_open_err:
    return res;
}

void dev_hidraw_close(dev_hidraw_t *const inout_dev) {
    close(inout_dev->fd);
    free(inout_dev);
}

int dev_hidraw_get_fd(const dev_hidraw_t* in_dev) {
    if (in_dev == NULL) {
        return -EINVAL;
    }

    return in_dev->fd;
}

int16_t dev_hidraw_get_pid(const dev_hidraw_t* in_dev) {
    return in_dev->info.product;
}

int16_t dev_hidraw_get_vid(const dev_hidraw_t* in_dev) {
    return in_dev->info.vendor;
}
