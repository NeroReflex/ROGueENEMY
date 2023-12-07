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

void gamepad_status_qam_quirk(gamepad_status_t *const gamepad_stats) {
    static struct timeval press_time;
    if (gamepad_stats->flags & GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER) {
        struct timeval now;
        gettimeofday(&now, NULL);

        // Calculate elapsed time in milliseconds
        const int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if (gamepad_stats->center) {
            // If the center button is pressed and at least X ms have passed
            if (elapsed_time >= PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS) {
                gamepad_stats->center = 0;
                gamepad_stats->flags &= ~GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
            }
        } else {
            // If the center button is pressed
            gamepad_stats->center = 1;
            gettimeofday(&press_time, NULL);
        }
    } else if (gamepad_stats->flags & GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM) {
        struct timeval now;
        gettimeofday(&now, NULL);

        static int releasing = 0;

        // Calculate elapsed time in milliseconds
        int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if ((gamepad_stats->center) && (!gamepad_stats->cross)) {
            if ((!releasing) && (elapsed_time >= PRESS_TIME_BEFORE_CROSS_BUTTON_MS)) {
                gamepad_stats->center = 1;
                gamepad_stats->cross = 1;
                press_time = now;
            } else if ((releasing) && (elapsed_time >= PRESS_TIME_AFTER_CROSS_BUTTON_MS)) {
                gamepad_stats->center = 0;
                gamepad_stats->cross = 0;
                press_time = now;
                gamepad_stats->flags &= ~GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
            }
        } else if ((gamepad_stats->center) && (gamepad_stats->cross)) {
            if (elapsed_time >= PRESS_TIME_CROSS_BUTTON_MS) {
                gamepad_stats->center = 1;
                gamepad_stats->cross = 0;
                releasing = 1;
                press_time = now;
            }
        } else {
            gamepad_stats->center = 1;
            gamepad_stats->cross = 0;
            releasing = 0;
            gettimeofday(&press_time, NULL);
        }
    }
}

void gamepad_status_qam_quirk_ext_time(gamepad_status_t *const gamepad_stats, struct timeval *now) {
    static struct timeval press_time;
    if (gamepad_stats->flags & GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER) {

        // Calculate elapsed time in milliseconds
        const int64_t elapsed_time = (now->tv_sec - press_time.tv_sec) * 1000 +
                               (now->tv_usec - press_time.tv_usec) / 1000;

        if (gamepad_stats->center) {
            // If the center button is pressed and at least X ms have passed
            if (elapsed_time >= PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS) {
                gamepad_stats->center = 0;
                gamepad_stats->flags &= ~GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
            }
        } else {
            // If the center button is pressed
            gamepad_stats->center = 1;
            gettimeofday(&press_time, NULL);
        }
    } else if (gamepad_stats->flags & GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM) {
        struct timeval now;
        gettimeofday(&now, NULL);

        static int releasing = 0;

        // Calculate elapsed time in milliseconds
        int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if ((gamepad_stats->center) && (!gamepad_stats->cross)) {
            if ((!releasing) && (elapsed_time >= PRESS_TIME_BEFORE_CROSS_BUTTON_MS)) {
                gamepad_stats->center = 1;
                gamepad_stats->cross = 1;
                press_time = now;
            } else if ((releasing) && (elapsed_time >= PRESS_TIME_AFTER_CROSS_BUTTON_MS)) {
                gamepad_stats->center = 0;
                gamepad_stats->cross = 0;
                press_time = now;
                gamepad_stats->flags &= ~GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
            }
        } else if ((gamepad_stats->center) && (gamepad_stats->cross)) {
            if (elapsed_time >= PRESS_TIME_CROSS_BUTTON_MS) {
                gamepad_stats->center = 1;
                gamepad_stats->cross = 0;
                releasing = 1;
                press_time = now;
            }
        } else {
            gamepad_stats->center = 1;
            gamepad_stats->cross = 0;
            releasing = 0;
            gettimeofday(&press_time, NULL);
        }
    }
}
