#pragma once

#include "rogue_enemy.h"

#define MESSAGE_FLAGS_HANDLE_DONE 0x00000001U

typedef struct message {
    struct input_event* ev;
    
    uint32_t ev_count;
    
    size_t ev_size;

    volatile uint32_t flags;
} message_t;

#define INPUT_FILTER_FLAGS_NONE             0x00000000U
#define INPUT_FILTER_FLAGS_DO_NOT_EMIT      0x00000001U
#define INPUT_FILTER_FLAGS_PRESERVE_TIME    0x00000002U

typedef uint32_t (*input_filter_t)(struct input_event*, size_t*, uint32_t*);