#include "logic.h"
#include "platform.h"
#include "queue.h"
#include "virt_ds4.h"
#include <sys/time.h>

static const char* configuration_file = "~/.config/ROGueENEMY/config";

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
    
    const int virt_ds4_thread_creation = pthread_create(&logic->virt_ds4_thread, NULL, virt_ds4_thread_func, (void*)(logic));
	if (virt_ds4_thread_creation != 0) {
		fprintf(stderr, "Error creating virtual DualShock4 thread: %d. Will use evdev as output.\n", virt_ds4_thread_creation);

        logic->gamepad_output = GAMEPAD_OUTPUT_EVDEV;
	} else {
        printf("Creation of virtual DualShock4 succeeded: using it as the defout output.\n");
        logic->flags |= LOGIC_FLAGS_VIRT_DS4_ENABLE;
        logic->gamepad_output = GAMEPAD_OUTPUT_DS4;
    }

    logic->restore_to = logic->gamepad_output;

    if (queue_init_res < 0) {
        fprintf(stderr, "Unable to create queue: %d\n", queue_init_res);
        return queue_init_res;
    }

    const int init_platform_res = init_platform(&logic->platform);
    if (init_platform_res == 0) {
        printf("RC71L platform correctly initialized\n");

        logic->flags |= LOGIC_FLAGS_PLATFORM_ENABLE;

        if (is_mouse_mode(&logic->platform)) {
            printf("Gamepad output will default to evdev when the controller is set in mouse mode\n");
            logic->gamepad_output = GAMEPAD_OUTPUT_EVDEV;
        }
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

    if (logic->gamepad.flags & GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER) {
        static struct timeval press_time;
        struct timeval now;
        gettimeofday(&now, NULL);

        // Calculate elapsed time in milliseconds
        int64_t elapsed_time = (now.tv_sec - press_time.tv_sec) * 1000 +
                               (now.tv_usec - press_time.tv_usec) / 1000;

        if (logic->gamepad.center) {
            // If the center button is pressed and at least 500ms have passed
            if (elapsed_time >= PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS) {
                printf("Releasing center button\n");
                logic->gamepad.center = 0;
                logic->gamepad.flags &= ~GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;
            }
        } else {
            // If the center button is pressed
            printf("Pressing center button\n");
            logic->gamepad.center = 1;
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