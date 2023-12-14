#pragma once

#include "input_dev.h"

int xbox360_ev_map(
    const dev_in_settings_t *const conf,
    const evdev_collected_t *const coll,
    in_message_t *const messages,
    size_t messages_len,
    void* user_data
);
