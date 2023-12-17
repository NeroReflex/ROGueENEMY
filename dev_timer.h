#pragma once

#include "input_dev.h"

typedef struct dev_timer {
    struct itimerspec timer_spec;
    
    int fd;
} dev_timer_t;

int dev_timer_open(
    const timer_filters_t *const in_filters,
    dev_timer_t **const out_dev
);

void dev_timer_close(dev_timer_t *const inout_dev);

int dev_timer_get_fd(const dev_timer_t *const in_dev);
