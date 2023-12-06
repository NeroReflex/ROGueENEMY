#pragma once

#include "input_dev.h"

typedef struct xbox360_settings {
    bool nintendo_layout;
} xbox360_settings_t;

void xbox360_ev_map(const evdev_collected_t *const e, int in_messages_pipe_fd, void* user_data);