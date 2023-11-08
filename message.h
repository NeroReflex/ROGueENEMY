#pragma once

#include "rogue_enemy.h"

#define MESSAGE_FLAGS_HANDLE_DONE 0x00000001U

typedef struct message {
    struct input_event* ev;
    
    uint32_t ev_count;
    
    size_t ev_size;

    volatile uint32_t flags;
} message_t;