#include "dev_out.h"

void *dev_out_thread_func(void *ptr) {
    dev_out_data_t *const dev_out = (dev_out_data_t*)ptr;

    dev_out_gamepad_device_t current_gamepad = dev_out->gamepad;

    int current_gamepad_fd = -1;
    int current_keyboard_fd = -1;
    int current_mouse_fd = -1;

    for (;;) {
        

        if (current_gamepad == GAMEPAD_DUALSENSE) {

        } else if (current_gamepad == GAMEPAD_DUALSHOCK) {

        }
    }

    return NULL;
}
