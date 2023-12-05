#pragma once

#include "platform.h"
#include "queue.h"
#include "settings.h"
#include "devices_status.h"

#define PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS     80
#define PRESS_TIME_BEFORE_CROSS_BUTTON_MS                   250
#define PRESS_TIME_CROSS_BUTTON_MS                          80
#define PRESS_TIME_AFTER_CROSS_BUTTON_MS                    180

#define GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER  0x00000001U
#define GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM             0x00000002U

#define LOGIC_FLAGS_PLATFORM_ENABLE         0x00000010U
#define LOGIC_FLAGS_TERMINATION_REQUESTED   0x80000000U

typedef struct rumble_message {
    uint16_t strong_magnitude;
    uint16_t weak_magnitude;
} rumble_message_t;

typedef struct logic {

    rc71l_platform_t platform;

    devices_status_t dev_stats;

    queue_t input_queue;

    pthread_t virt_dev_thread;
    bool virt_dev_thread_running;

    volatile uint32_t flags;

    queue_t rumble_events_queue;

    controller_settings_t controller_settings;

} logic_t;

int logic_create(logic_t *const logic);

int is_rc71l_ready(const logic_t *const logic);

void logic_request_termination(logic_t *const logic);

int logic_termination_requested(logic_t *const logic);

/**
 * This function starts the output thread for the user-chosed gamepad.
 *
 * Before starting the thread this function will also modify the status of "connected"
 * so that the started thread won't exit immediatly.
 * This function can only be called when the mutex is not needed:
 *     in practice this is true when no output thread is active
 */
int logic_start_output_dev_thread(logic_t *const logic);

/**
 * This function starts the output thread for the keyboard&mouse.
 *
 * Before starting the thread this function will also modify the status of "connected"
 * so that the started thread won't exit immediatly.
 * 
 * This function can only be called when the mutex is not needed:
 *     in practice this is true when no output thread is active
 */
int logic_start_output_mouse_kbd_thread(logic_t *const logic);

void logic_terminate_output_thread(logic_t *const logic);

/*
int logic_copy_gamepad_status(logic_t *const logic, gamepad_status_t *const out);

int logic_begin_status_update(logic_t *const logic);

void logic_end_status_update(logic_t *const logic);
*/

