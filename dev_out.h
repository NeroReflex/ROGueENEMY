#pragma once

#include "ipc.h"
#include "message.h"
#include "devices_status.h"

#define DEV_OUT_FLAG_EXIT 0x00000001U

typedef enum dev_out_gamepad_device {
    GAMEPAD_DUALSENSE,
    GAMEPAD_DUALSHOCK,
    GAMEPAD_XBOX,
} dev_out_gamepad_device_t;

typedef struct dev_out_data {

    ipc_t communication;

    dev_out_gamepad_device_t gamepad;

    devices_status_t dev_stats;

    volatile uint32_t flags;

} dev_out_data_t;

void *dev_out_thread_func(void *ptr);
