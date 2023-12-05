#include "logic.h"
#include "platform.h"
#include "queue.h"
#include "virt_ds4.h"
#include "virt_ds5.h"
#include "virt_evdev.h"
#include "virt_mouse_kbd.h"

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

int logic_create(logic_t *const logic) {
    int ret = 0;

    logic->flags = 0x00000000U;

    init_config(&logic->controller_settings);
    const int fill_config_res = fill_config(&logic->controller_settings, configuration_file);
    if (fill_config_res != 0) {
        fprintf(stderr, "Unable to fill configuration from file %s -- defaults will be used\n", configuration_file);
    }

    devices_status_init(&logic->dev_stats);

    ret = queue_init(&logic->rumble_events_queue, 1);
    if (ret != 0) {
        fprintf(stderr, "Unable to create the rumble events queue: %d\n", ret);
        goto logic_create_err;
    }

    ret = pthread_mutex_init(&logic->dev_stats.mutex, NULL);
    if (ret != 0) {
        fprintf(stderr, "Unable to create mutex: %d\n", ret);
        goto logic_create_err;
    }

    const int queue_init_res = queue_init(&logic->input_queue, 128);

    if (queue_init_res < 0) {
        fprintf(stderr, "Unable to create queue: %d\n", queue_init_res);
        return queue_init_res;
    }

    bool lizard_thread_started = false;
    const int init_platform_res = init_platform(&logic->platform);
    if (init_platform_res == 0) {
        printf("RC71L platform correctly initialized\n");

        logic->flags |= LOGIC_FLAGS_PLATFORM_ENABLE;

        if (is_mouse_mode(&logic->platform)) {
            printf("Device is in lizard mode\n");
            // TODO: start the appropriate output thread

            lizard_thread_started = true;
        }
    } else {
        fprintf(stderr, "Unable to initialize Asus RC71L MCU: %d\n", init_platform_res);
    }

    if (!lizard_thread_started) {
        logic_start_output_dev_thread(logic);
    }

logic_create_err:
    return ret;
}

int is_rc71l_ready(const logic_t *const logic) {
    return logic->flags & LOGIC_FLAGS_PLATFORM_ENABLE;
}

void logic_terminate_output_thread(logic_t *const logic) {
    if (logic->virt_dev_thread_running) {
        void* thread_return = NULL;
        pthread_join(logic->virt_dev_thread, &thread_return);
    }
}

int logic_start_output_mouse_kbd_thread(logic_t *const logic) {
    // TODO: logic->dev_stats.mouse_kbd.connected = true;
    const int ret = pthread_create(&logic->virt_dev_thread, NULL, virt_mouse_kbd_thread_func, (void*)(&logic->dev_stats));

    logic->virt_dev_thread_running = ret == 0;

    return ret;
}

int logic_start_output_dev_thread(logic_t *const logic) {
    logic->dev_stats.gamepad.connected = true;

    int ret = -EINVAL;
    switch (logic->controller_settings.gamepad_output_device) {
        
    case 0:
        ret = pthread_create(&logic->virt_dev_thread, NULL, virt_evdev_thread_func, (void*)(&logic->dev_stats));
        break;

    case 1:
        ret = pthread_create(&logic->virt_dev_thread, NULL, virt_ds5_thread_func, (void*)(&logic->dev_stats));
        break;

    case 2:
        ret = pthread_create(&logic->virt_dev_thread, NULL, virt_ds4_thread_func, (void*)(&logic->dev_stats));
        break;
    
    default:
        fprintf(stderr, "Invalid output device specified\n");
        ret = -EINVAL;
        break;

    }

    logic->virt_dev_thread_running = ret == 0;

    return ret;
}

/*
int logic_copy_gamepad_status(logic_t *const logic, gamepad_status_t *const out) {
    int res = 0;

    res = pthread_mutex_lock(&logic->gamepad_mutex);
    if (res != 0) {
        goto logic_copy_gamepad_status_err;
    }

    static struct timeval press_time;
    if (logic->gamepad.flags & GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER) {
        struct timeval now;
        gettimeofday(&now, NULL);

        // Calculate elapsed time in milliseconds
        int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if (logic->gamepad.center) {
            // If the center button is pressed and at least X ms have passed
            if (elapsed_time >= PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS) {
                logic->gamepad.center = 0;
                logic->gamepad.flags &= ~GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
            }
        } else {
            // If the center button is pressed
            logic->gamepad.center = 1;
            gettimeofday(&press_time, NULL);
        }
    } else if (logic->gamepad.flags & GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM) {
        struct timeval now;
        gettimeofday(&now, NULL);

        static int releasing = 0;

        // Calculate elapsed time in milliseconds
        int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if ((logic->gamepad.center) && (!logic->gamepad.cross)) {
            if ((!releasing) && (elapsed_time >= PRESS_TIME_BEFORE_CROSS_BUTTON_MS)) {
                logic->gamepad.center = 1;
                logic->gamepad.cross = 1;
                press_time = now;
            } else if ((releasing) && (elapsed_time >= PRESS_TIME_AFTER_CROSS_BUTTON_MS)) {
                logic->gamepad.center = 0;
                logic->gamepad.cross = 0;
                press_time = now;
                logic->gamepad.flags &= ~GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM;
            }
        } else if ((logic->gamepad.center) && (logic->gamepad.cross)) {
            if (elapsed_time >= PRESS_TIME_CROSS_BUTTON_MS) {
                logic->gamepad.center = 1;
                logic->gamepad.cross = 0;
                releasing = 1;
                press_time = now;
            }
        } else {
            logic->gamepad.center = 1;
            logic->gamepad.cross = 0;
            releasing = 0;
            gettimeofday(&press_time, NULL);
        }
    }

    *out = logic->gamepad;

    pthread_mutex_unlock(&logic->gamepad_mutex);

logic_copy_gamepad_status_err:
    return res;
}

int logic_begin_status_update(logic_t *const logic) {
    int res = 0;

    res = pthread_mutex_lock(&logic->gamepad_mutex);
    if (res != 0) {
        goto logic_begin_status_update_err;
    }

logic_begin_status_update_err:
    return res;
}

void logic_end_status_update(logic_t *const logic) {
    pthread_mutex_unlock(&logic->gamepad_mutex);
}
*/

void logic_request_termination(logic_t *const logic) {
    logic->flags |= LOGIC_FLAGS_TERMINATION_REQUESTED;
}

int logic_termination_requested(logic_t *const logic) {
    return (logic->flags & LOGIC_FLAGS_TERMINATION_REQUESTED) != 0;
}
