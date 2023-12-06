#include "logic.h"
#include "platform.h"
#include "queue.h"
#include "virt_ds4.h"
#include "virt_ds5.h"

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

int logic_create(logic_t *const logic) {
    logic->flags = 0x00000000U;

    memset(logic->gamepad.joystick_positions, 0, sizeof(logic->gamepad.joystick_positions));
    logic->gamepad.dpad = 0x00;
    logic->gamepad.l2_trigger = 0;
    logic->gamepad.r2_trigger = 0;
    logic->gamepad.triangle = 0;
    logic->gamepad.circle = 0;
    logic->gamepad.cross = 0;
    logic->gamepad.square = 0;
    logic->gamepad.r3 = 0;
    logic->gamepad.r3 = 0;
    logic->gamepad.option = 0;
    logic->gamepad.share = 0;
    logic->gamepad.center = 0;
    logic->gamepad.touchpad_press = 0;
    logic->gamepad.touchpadX = 0;
    logic->gamepad.touchpadY = 0;
    logic->gamepad.r4 = 0;
    logic->gamepad.l4 = 0;
    logic->gamepad.r5 = 0;
    logic->gamepad.l5 = 0;
    logic->gamepad.rumble_events_count = 0;
    memset(logic->gamepad.gyro, 0, sizeof(logic->gamepad.gyro));
    memset(logic->gamepad.accel, 0, sizeof(logic->gamepad.accel));
    logic->gamepad.flags = 0;

    const int mutex_creation_res = pthread_mutex_init(&logic->gamepad_mutex, NULL);
    if (mutex_creation_res != 0) {
        fprintf(stderr, "Unable to create mutex: %d\n", mutex_creation_res);
        return mutex_creation_res;
    }

    const int queue_init_res = queue_init(&logic->input_queue, 128);
    
    // const int virt_ds4_thread_creation = pthread_create(&logic->virt_ds4_thread, NULL, virt_ds4_thread_func, (void*)(logic));
	// if (virt_ds4_thread_creation != 0) {
	// 	fprintf(stderr, "Error creating virtual DualShock4 thread: %d. Will use evdev as output.\n", virt_ds4_thread_creation);

    //     logic->gamepad_output = GAMEPAD_OUTPUT_EVDEV;
	// } else {
    //     printf("Creation of virtual DualShock4 succeeded: using it as the defaut output.\n");
    //     logic->flags |= LOGIC_FLAGS_VIRT_DS4_ENABLE;
    //     logic->gamepad_output = GAMEPAD_OUTPUT_DS4;
    // }

    const int virt_ds5_thread_creation = pthread_create(&logic->virt_ds5_thread, NULL, virt_ds5_thread_func, (void*)(logic));
	if (virt_ds5_thread_creation != 0) {
		fprintf(stderr, "Error creating virtual DualSense thread: %d.\n", virt_ds5_thread_creation);
	} 
    // else {
    //     printf("Creation of virtual DualShock4 succeeded: using it as the defaut output.\n");
    //     logic->flags |= LOGIC_FLAGS_VIRT_DS5_ENABLE;
    //     logic->gamepad_output = GAMEPAD_OUTPUT_DS5;
    // }

    if (queue_init_res < 0) {
        fprintf(stderr, "Unable to create queue: %d\n", queue_init_res);
        return queue_init_res;
    }

    const int init_platform_res = init_platform(&logic->platform);
    if (init_platform_res == 0) {
        // printf("RC71L platform correctly initialized\n");

        logic->flags |= LOGIC_FLAGS_PLATFORM_ENABLE;

        // if (is_mouse_mode(&logic->platform)) {
        //     printf("Gamepad output will default to evdev when the controller is set in mouse mode.\n");
        //     logic->gamepad_output = GAMEPAD_OUTPUT_EVDEV;
        // } else if (is_gamepad_mode(&logic->platform)) {
        //     logic->gamepad_output = (logic->flags & LOGIC_FLAGS_VIRT_DS5_ENABLE) ? GAMEPAD_OUTPUT_DS5 : ((logic->flags & LOGIC_FLAGS_VIRT_DS4_ENABLE) ? GAMEPAD_OUTPUT_DS4: GAMEPAD_OUTPUT_EVDEV);
        // } else if (is_macro_mode(&logic->platform)) {
        //     logic->gamepad_output = (logic->flags & LOGIC_FLAGS_VIRT_DS4_ENABLE) ? GAMEPAD_OUTPUT_DS4 : GAMEPAD_OUTPUT_EVDEV;
        // }
        logic->gamepad_output = GAMEPAD_OUTPUT_DS5;

        printf("Gamepad output is %d\n", (int)logic->gamepad_output);
    } else {
        fprintf(stderr, "Unable to initialize Asus RC71L MCU: %d\n", init_platform_res);
    }

    queue_init(&logic->rumble_events_queue, 1);

    init_config(&logic->controller_settings);
    const int fill_config_res = fill_config(&logic->controller_settings, configuration_file);
    if (fill_config_res != 0) {
        fprintf(stderr, "Unable to fill configuration from file %s\n", configuration_file);
    }

    return 0;
}

int is_rc71l_ready(const logic_t *const logic) {
    return logic->flags & LOGIC_FLAGS_PLATFORM_ENABLE;
}

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

void logic_request_termination(logic_t *const logic) {
    logic->flags |= LOGIC_FLAGS_TERMINATION_REQUESTED;
}

int logic_termination_requested(logic_t *const logic) {
    return (logic->flags & LOGIC_FLAGS_TERMINATION_REQUESTED) != 0;
}