#pragma once

#include "queue.h"

#define INPUT_DEV_CTRL_FLAG_EXIT 0x00000001U

typedef enum input_dev_type {
    input_dev_type_uinput,
    input_dev_type_iio,
} input_dev_type_t;

typedef struct uinput_filters {
    const char* name;
} uinput_filters_t;

typedef struct iio_filters {
    const char* name;
} iio_filters_t;

typedef struct input_dev {
    input_dev_type_t dev_type;

    const uinput_filters_t* ev_filters;
    const iio_filters_t* iio_filters;

    volatile uint32_t crtl_flags;

    queue_t *queue;
} input_dev_t;

void *input_dev_thread_func(void *ptr);

int open_and_hide_input();