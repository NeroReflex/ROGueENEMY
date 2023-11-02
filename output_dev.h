#pragma once

#include <inttypes.h>
#include <pthread.h>

#define OUTPUT_DEV_CTRL_FLAG_EXIT 0x00000001U
#define OUTPUT_DEV_CTRL_FLAG_DATA 0x00000002U

typedef struct output_dev {
    int fd;

    pthread_mutex_t ctrl_mutex;
    uint32_t crtl_flags;
} output_dev_t;