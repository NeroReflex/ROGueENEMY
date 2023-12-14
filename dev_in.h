#pragma once

#include "ipc.h"
#include "message.h"
#include "input_dev.h"
#include "settings.h"

#define MAX_IN_MESSAGES 8

#define DEV_IN_FLAG_EXIT 0x00000001U

typedef struct dev_in_data {
    size_t max_messages_in_flight;

    // the timeout (in missileconds)
    uint64_t timeout_ms;

    // declarations of devices to monitor
    input_dev_composite_t *input_dev_decl;

    ipc_t communication;

    dev_in_settings_t settings;

    volatile uint32_t flags;

} dev_in_data_t;

void *dev_in_thread_func(void *ptr);
