#pragma once

#include "message.h"

#undef INCLUDE_INPUT_DEBUG
#undef IGNORE_INPUT_SCAN

#define MAX_COLLECTED_EVDEV_EVENTS 16
#define MAX_INPUT_DEVICES 8

typedef struct evdev_collected {
    struct input_event ev[MAX_COLLECTED_EVDEV_EVENTS];
    size_t ev_count;
} evdev_collected_t;

/**
 * A function with this signature grapbs input_event data and sends to the pipe messages
 * constructed from that data.
 */
typedef void (*ev_map)(const evdev_collected_t *const e, int in_messages_pipe_fd, void* user_data);

typedef enum input_dev_type {
    input_dev_type_uinput,
    input_dev_type_iio,
    input_dev_type_hidraw,
} input_dev_type_t;

typedef struct hidraw_filters {
    const int16_t pid;
    const int16_t vid;
    const uint16_t rdesc_size; // wc -c < /sys/class/hidraw/hidraw0/device/report_descriptor
} hidraw_filters_t;

typedef struct uinput_filters {
    const char name[256];
} uinput_filters_t;

typedef struct iio_filters {
    const char name[256];
} iio_filters_t;

typedef int (*hidraw_set_leds)(uint8_t r, uint8_t g, uint8_t b, void* user_data);

typedef int (*hidraw_rumble)(uint8_t left_motor, uint8_t right_motor, void* user_data);

typedef struct hidraw_callbacks {
    hidraw_set_leds leds_callback;
    hidraw_rumble rumble_callback;
} hidraw_callbacks_t;

typedef struct input_dev {
    input_dev_type_t dev_type;

    union {
        uinput_filters_t ev;
        iio_filters_t iio;
    } filters;

    void* user_data;

    ev_map ev_input_map_fn;

} input_dev_t;

typedef int (*platform_init)(void** platform_data);

typedef void (*platform_deinit)(void** platform_data);

typedef int (*platform_leds)(uint8_t r, uint8_t g, uint8_t b, void* platform_data);

typedef struct input_dev_composite {

    const input_dev_t* dev[MAX_INPUT_DEVICES];

    size_t dev_count;

    platform_leds leds_fn;

    platform_init init_fn;

    platform_deinit deinit_fn;

} input_dev_composite_t;

uint32_t input_filter_imu_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags);

uint32_t input_filter_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags);

uint32_t input_filter_asus_kb(struct input_event*, size_t*, uint32_t*, uint32_t* flags);
