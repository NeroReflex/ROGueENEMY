#pragma once

#include "logic.h"

#undef INCLUDE_TIMESTAMP
#undef INCLUDE_OUTPUT_DEBUG

typedef struct output_dev {
    logic_t *logic;

    // this pipe is reserved for reporting in_message_t
    int in_message_pipe_fd;

    // this messages is reserved for receiving out_message_t
    int out_message_pipe_fd;

} output_dev_t;

void *output_dev_thread_func(void *ptr);
