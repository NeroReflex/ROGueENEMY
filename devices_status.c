#include "devices_status.h"

void kbd_status_init(keyboard_status_t *const stats) {
    stats->connected = true;
}

void gamepad_status_init(gamepad_status_t *const stats) {
    stats->connected = true;
    stats->joystick_positions[0][0] = 0;
    stats->joystick_positions[0][1] = 0;
    stats->joystick_positions[1][0] = 0;
    stats->joystick_positions[1][1] = 0;
    stats->dpad = 0x00;
    stats->l2_trigger = 0;
    stats->r2_trigger = 0;
    stats->triangle = 0;
    stats->circle = 0;
    stats->cross = 0;
    stats->square = 0;
    stats->r3 = 0;
    stats->r3 = 0;
    stats->option = 0;
    stats->share = 0;
    stats->center = 0;
    stats->r4 = 0;
    stats->l4 = 0;
    stats->r5 = 0;
    stats->l5 = 0;
    stats->motors_intensity[0] = 0;
    stats->motors_intensity[1] = 0;
    stats->rumble_events_count = 0;
    stats->gyro[0] = 0;
    stats->gyro[1] = 0;
    stats->gyro[2] = 0;
    stats->accel[0] = 0;
    stats->accel[1] = 0;
    stats->accel[2] = 0;
    stats->flags = 0;
}

void devices_status_init(devices_status_t *const stats) {
    gamepad_status_init(&stats->gamepad);
    kbd_status_init(&stats->kbd);
    // TODO: mouse init
}