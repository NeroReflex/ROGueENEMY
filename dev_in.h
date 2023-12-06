#pragma once

#include "message.h"
#include "input_dev.h"

typedef struct dev_in_data {
    size_t max_messages_in_flight;

    // the timeout (in missileconds)
    uint64_t timeout_ms;

    // declarations of devices to monitor
    input_dev_t *input_dev_decl;

    // number of devices to monitor
    size_t input_dev_cnt;

    // this pipe is reserved for reporting in_message_t
    int in_message_pipe_fd;

    // this messages is reserved for receiving out_message_t
    int out_message_pipe_fd;

} dev_in_data_t;

void *dev_in_thread_func(void *ptr);
