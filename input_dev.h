#pragma once

#include "rogue_enemy.h"

#define INPUT_DEV_CTRL_FLAG_EXIT 0x00000001U

typedef enum input_dev_type {
    input_dev_type_uinput,
    input_dev_type_iio,
} input_dev_type_t;

typedef struct input_dev {
    input_dev_type_t dev_type;

    volatile uint32_t crtl_flags;
} input_dev_t;

void *input_dev_thread_func(void *ptr);

int open_and_hide_input();