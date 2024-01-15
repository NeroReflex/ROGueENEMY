#include "dev_timer.h"

int dev_timer_open(
    const timer_filters_t *const in_filters,
    dev_timer_t **const out_dev
) {
    int res = -ENODEV;

    *out_dev = malloc(sizeof(dev_timer_t));
    if (*out_dev == NULL) {
        res = -ENOMEM;
        goto dev_timer_open_err;
    }

    memset(*out_dev, 0, sizeof(dev_timer_t));

    const int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd < 0) {
        res = errno < 0 ? errno : -1 * errno;
        if (res == 0) {
            res = fd;
        }
        goto dev_timer_open_err;
    }

    (*out_dev)->fd = fd;
    if (in_filters->ticktime_ms != 0) {
        (*out_dev)->timer_spec.it_value.tv_sec = in_filters->ticktime_ms / (__time_t)1000;
        (*out_dev)->timer_spec.it_value.tv_nsec = (in_filters->ticktime_ms % (__syscall_slong_t)1000) * (__syscall_slong_t)1000000;
        (*out_dev)->timer_spec.it_interval.tv_sec = in_filters->ticktime_ms / (__time_t)1000;
        (*out_dev)->timer_spec.it_interval.tv_nsec = (in_filters->ticktime_ms % (__syscall_slong_t)1000) * (__syscall_slong_t)1000000;
    } else {
        (*out_dev)->timer_spec.it_value.tv_sec = 0;
        (*out_dev)->timer_spec.it_value.tv_nsec = in_filters->ticktime_ns;
        (*out_dev)->timer_spec.it_interval.tv_sec = 0;
        (*out_dev)->timer_spec.it_interval.tv_nsec = in_filters->ticktime_ns;
    }

    if (timerfd_settime((*out_dev)->fd, 0, &(*out_dev)->timer_spec, NULL) < 0) {
        res = errno < 0 ? errno : -1 * errno;
        if (res == 0) {
            res = -EIO;
        }
        goto dev_timer_open_err;
    }

    res = 0;

dev_timer_open_err:
    if (res != 0) {
        if (fd > 0) {
            close(fd);
        }

        free(*out_dev);
    }

    return res;
}

void dev_timer_close(dev_timer_t *const inout_dev) {
    close(inout_dev->fd);
}

int dev_timer_get_fd(const dev_timer_t *const in_dev) {
    return in_dev->fd;
}
