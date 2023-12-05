#pragma once

#include "queue.h"
#include "logic.h"

#undef INCLUDE_TIMESTAMP
#undef INCLUDE_OUTPUT_DEBUG

typedef struct output_dev {
    logic_t *logic;
} output_dev_t;

void *output_dev_thread_func(void *ptr);
