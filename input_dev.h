#pragma once

#include <inttypes.h>
#include <pthread.h>

#define INPUT_DEV_CTRL_FLAG_EXIT 0x00000001U
#define INPUT_DEV_CTRL_FLAG_DATA 0x00000002U

typedef struct input_dev {
    pthread_mutex_t ctrl_mutex;
    uint32_t crtl_flags;
} input_dev_t;