#pragma once

#include "rogue_enemy.h"

#undef VIRT_EVDEV_DEBUG

// Emulates a "Generic" controller:
#define OUTPUT_DEV_NAME             "ROGueENEMY"
#define OUTPUT_DEV_VENDOR_ID        0x108c
#define OUTPUT_DEV_PRODUCT_ID       0x0323
#define OUTPUT_DEV_VERSION          0x0111

#define PHYS_STR "00:11:22:33:44:55"

void *virt_mouse_kbd_thread_func(void *ptr);
