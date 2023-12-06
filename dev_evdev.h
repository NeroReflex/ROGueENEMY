#pragma once

#include "input_dev.h"

int dev_evdev_open(
    const uinput_filters_t *const in_filters,
    struct libevdev* out_evdev
);

void dev_evdev_close(struct libevdev* out_evdev);
