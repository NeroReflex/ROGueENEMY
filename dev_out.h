#pragma once

#include "message.h"
#include "devices_status.h"

typedef enum dev_out_gamepad_device {
    GAMEPAD_DUALSENSE,
    GAMEPAD_DUALSHOCK,
    GAMEPAD_XBOX,
} dev_out_gamepad_device_t;

typedef struct dev_out_data {

    // this pipe is reserved for reporting in_message_t
    int in_message_pipe_fd;

    // this messages is reserved for receiving out_message_t
    int out_message_pipe_fd;

    dev_out_gamepad_device_t gamepad;

    devices_status_t dev_stats;

} dev_out_data_t;

void *dev_out_thread_func(void *ptr);
