#pragma once

#include "input_dev.h"

typedef struct xbox360_settings {
    bool nintendo_layout;
} xbox360_settings_t;

int xbox360_ev_map(const evdev_collected_t *const coll, in_message_t *const messages, size_t messages_len, void* user_data);
