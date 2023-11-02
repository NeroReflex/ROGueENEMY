#pragma once

#include "rogue_enemy.h"

#define INPUT_DEV_CTRL_FLAG_EXIT 0x00000001U

typedef struct input_dev {
    volatile uint32_t crtl_flags;
} input_dev_t;

void *input_dev_thread_func(void *ptr);

int open_and_hide_input();