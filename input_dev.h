#pragma once

#include "message.h"
#include "logic.h"

#undef INCLUDE_INPUT_DEBUG
#undef IGNORE_INPUT_SCAN

typedef uint32_t (*ev_input_filter_t)(struct input_event*, size_t*, uint32_t*, uint32_t*);

typedef enum input_dev_type {
    input_dev_type_uinput,
    input_dev_type_iio,
} input_dev_type_t;

typedef struct uinput_filters {
    const char name[256];
} uinput_filters_t;

typedef struct iio_filters {
    const char name[256];
} iio_filters_t;

typedef struct input_dev {
    input_dev_type_t dev_type;

    union {
        uinput_filters_t ev;
        iio_filters_t iio;
    } filters;

    ev_input_filter_t ev_input_filter_fn;

} input_dev_t;

uint32_t input_filter_imu_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags);

uint32_t input_filter_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags);

uint32_t input_filter_asus_kb(struct input_event*, size_t*, uint32_t*, uint32_t* flags);
