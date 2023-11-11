#pragma once

#include "queue.h"

// Emulates a "Generic" controller:
#define OUTPUT_DEV_NAME             "Generic Controller XD"
#define OUTPUT_DEV_VENDOR_ID        0x108c
#define OUTPUT_DEV_PRODUCT_ID       0x0323

/*
//Emulates an Xbox one wireless controller:
#define OUTPUT_DEV_NAME             "Xbox Wireless Controller"
#define OUTPUT_DEV_VENDOR_ID        0x045e
#define OUTPUT_DEV_PRODUCT_ID       0x028e
*/

/*
// Emulates a DualShock controller
#define OUTPUT_DEV_NAME             "Sony Interactive Entertainment DualSense Wireless Controller"
#define OUTPUT_DEV_VENDOR_ID        0x054c
#define OUTPUT_DEV_PRODUCT_ID       0x0ce6
*/

#define OUTPUT_DEV_CTRL_FLAG_EXIT 0x00000001U
#define OUTPUT_DEV_CTRL_FLAG_DATA 0x00000002U

#define ACCEL_RANGE 512
#define GYRO_RANGE  2000 // max range is +/- 35 radian/s

#define GYRO_DEADZONE 1 // degrees/s to count as zero movement

#undef INCLUDE_TIMESTAMP
#define INCLUDE_OUTPUT_DEBUG

typedef enum output_dev_type {
    output_dev_gamepad,
    output_dev_imu,
} output_dev_type_t;

typedef struct output_dev {
    int fd;

    volatile uint32_t crtl_flags;

    queue_t *queue;
} output_dev_t;

int create_output_dev(const char* uinput_path, const char* name, output_dev_type_t type);

void *output_dev_thread_func(void *ptr);