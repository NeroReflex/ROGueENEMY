#pragma once

#include "input_dev.h"

typedef struct dev_hidraw {
    struct hidraw_devinfo info;

    struct hidraw_report_descriptor rdesc;

    int fd;
} dev_hidraw_t;

int dev_hidraw_open(
    const hidraw_filters_t *const in_filters,
    dev_hidraw_t **const out_dev
);

void dev_hidraw_close(dev_hidraw_t *const inout_dev);

int dev_hidraw_get_fd(const dev_hidraw_t *const in_dev);

int16_t dev_hidraw_get_pid(const dev_hidraw_t *const in_dev);

int16_t dev_hidraw_get_vid(const dev_hidraw_t *const in_dev);

int dev_hidraw_get_rdesc(const dev_hidraw_t *const in_dev, uint8_t *const out_buf_sz, size_t in_buf_sz, size_t *const out_rdesc_size);