#pragma once

#include "message.h"
#include "settings.h"

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
typedef int (*ev_map)(
    const dev_in_settings_t *const conf,
    const evdev_collected_t *const e,
    in_message_t *const messages,
    size_t messages_len,
    void* user_data
);
typedef void (*ev_timer)(
    const dev_in_settings_t *const conf,
    struct libevdev* evdev,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
);

typedef enum input_dev_type {
    input_dev_type_uinput,
    input_dev_type_iio,
    input_dev_type_hidraw,
    input_dev_type_timer,
} input_dev_type_t;

typedef struct hidraw_filters {
    const int16_t pid;
    const int16_t vid;
    const uint32_t rdesc_size; // wc -c < /sys/class/hidraw/hidraw0/device/report_descriptor
} hidraw_filters_t;

typedef struct uinput_filters {
    const char name[256];
} uinput_filters_t;

typedef struct iio_filters {
    const char name[256];
} iio_filters_t;

typedef int (*hidraw_map)(
    const dev_in_settings_t *const conf,
    int hidraw_fd,
    in_message_t *const messages,
    size_t messages_len,
    void* user_data
);

typedef int (*hidraw_set_leds)(
    const dev_in_settings_t *const conf,
    int hidraw_fd,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    void* user_data
);

typedef int (*hidraw_rumble)(
    const dev_in_settings_t *const conf,
    int hidraw_fd,
    uint8_t left_motor,
    uint8_t right_motor,
    void* user_data
);

typedef void (*hidraw_timer)(
    const dev_in_settings_t *const conf,
    int fd,
    const char* const timer_name,
    uint64_t expired,
    void* user_data
);

typedef struct hidraw_callbacks {
    hidraw_set_leds leds_callback;
    hidraw_rumble rumble_callback;
    hidraw_map map_callback;
    hidraw_timer timeout_callback;
} hidraw_callbacks_t;

typedef struct iio_settings {
    const char* const sampling_freq_hz;
    int8_t post_matrix[3][3];
} iio_settings_t;

typedef int (*timer_map)(const dev_in_settings_t *const conf, int timer_fd, uint64_t expirations, in_message_t *const messages, size_t messages_len, void* user_data);

typedef struct timer_callbacks {
    timer_map map_fn;
} timer_callbacks_t;

typedef struct ev_callbacks {
    ev_map input_map_fn;
    ev_timer timeout_callback;
} ev_callbacks_t;

typedef struct timer_filters {
    char name[128];
    uint64_t ticktime_ms;
    uint64_t ticktime_ns;
} timer_filters_t;

typedef struct input_dev {
    input_dev_type_t dev_type;

    union {
        uinput_filters_t ev;
        iio_filters_t iio;
        hidraw_filters_t hidraw;
        timer_filters_t timer;
    } filters;

    void* user_data;

    union input_dev_map {
        iio_settings_t iio_settings;
        ev_callbacks_t ev_callbacks;
        hidraw_callbacks_t hidraw_callbacks;
        timer_callbacks_t timer_callbacks;
    } map;

} input_dev_t;

typedef int (*platform_init)(const dev_in_settings_t *const conf, void** platform_data);

typedef void (*platform_deinit)(const dev_in_settings_t *const conf, void** platform_data);

typedef int (*platform_leds)(const dev_in_settings_t *const conf, uint8_t r, uint8_t g, uint8_t b, void* platform_data);

typedef struct input_dev_composite {

    const input_dev_t* dev[MAX_INPUT_DEVICES];

    size_t dev_count;

    platform_leds leds_fn;

    platform_init init_fn;

    platform_deinit deinit_fn;

} input_dev_composite_t;
