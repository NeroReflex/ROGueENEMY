#include "devices_status.h"
#include <pthread.h>

void kbd_status_init(keyboard_status_t *const stats) {
    stats->connected = true;

    stats->q = 0;
    stats->w = 0;
    stats->e = 0;
    stats->r = 0;
    stats->t = 0;
    stats->y = 0;
    stats->u = 0;
    stats->i = 0;
    stats->o = 0;
    stats->p = 0;
    stats->a = 0;
    stats->s = 0;
    stats->d = 0;
    stats->f = 0;
    stats->g = 0;
    stats->h = 0;
    stats->j = 0;
    stats->k = 0;
    stats->l = 0;
    stats->z = 0;
    stats->x = 0;
    stats->c = 0;
    stats->v = 0;
    stats->b = 0;
    stats->n = 0;
    stats->m = 0;

    stats->num_1 = 0;
    stats->num_2 = 0;
    stats->num_3 = 0;
    stats->num_4 = 0;
    stats->num_5 = 0;
    stats->num_6 = 0;
    stats->num_7 = 0;
    stats->num_8 = 0;
    stats->num_9 = 0;
    stats->num_0 = 0;

    stats->up = 0;
    stats->down = 0;
    stats->left = 0;
    stats->right = 0;

    stats->lctrl = 0;
}

void mouse_status_init(mouse_status_t *const stats) {
    stats->connected = true;

    stats->x = 0;
    stats->y = 0;
    stats->btn_left = 0;
    stats->btn_middle = 0;
    stats->btn_right = 0;
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
    stats->leds_events_count = 0;
    stats->leds_colors[0] = 0;
    stats->leds_colors[1] = 0;
    stats->leds_colors[2] = 0;
    stats->touchpad_touch_num = -1;
    stats->touchpad_x = 0;
    stats->touchpad_y = 0;
    stats->join_left_analog_and_gyroscope = 0;
    stats->join_right_analog_and_gyroscope = 0;
    stats->flags = 0;
}

void devices_status_init(devices_status_t *const stats) {
    pthread_mutex_init(&stats->mutex, NULL);
    gamepad_status_init(&stats->gamepad);
    kbd_status_init(&stats->kbd);
    mouse_status_init(&stats->mouse);
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

void gamepad_status_qam_quirk_ext_time(gamepad_status_t *const gamepad_stats) {
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
