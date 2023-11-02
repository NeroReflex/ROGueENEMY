#pragma once

#include "rogue_enemy.h"

#define INPUT_DEV_CTRL_FLAG_EXIT 0x00000001U

typedef struct input_dev {
    pthread_mutex_t ctrl_mutex;
    uint32_t crtl_flags;
} input_dev_t;

int open_and_hide_input();