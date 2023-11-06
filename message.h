#pragma once

#include "rogue_enemy.h"

#define MESSAGE_FLAGS_HANDLE_DONE 0x00000001U

typedef struct message {
    struct input_event ev;
    volatile uint32_t flags;
} message_t;