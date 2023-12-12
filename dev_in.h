#pragma once

#include "ipc.h"
#include "message.h"
#include "input_dev.h"

#define MAX_IN_MESSAGES 8

typedef struct dev_in_data {
    size_t max_messages_in_flight;

    // the timeout (in missileconds)
    uint64_t timeout_ms;

    // declarations of devices to monitor
    input_dev_composite_t *input_dev_decl;

    ipc_t communication;

} dev_in_data_t;

void *dev_in_thread_func(void *ptr);
