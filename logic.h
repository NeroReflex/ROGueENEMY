#pragma once

#include "platform.h"
#include "queue.h"

typedef struct gamepad_status {

    int32_t joystick_positions[2][2]; // [0 left | 1 right][x axis | y axis]

    uint8_t dpad; // 0x00 x - | 0x01 x -> | 0x02 x <- | 0x00 y - | 0x10 y ^ | 0x10 y . | 

    uint8_t l2_trigger;
    uint8_t r2_trigger;

    uint8_t triangle;
    uint8_t circle;
    uint8_t cross;
    uint8_t square;

    uint8_t l1;
    uint8_t r1;

    uint8_t r3;
    uint8_t l3;

    uint8_t option;
    uint8_t share;
    uint8_t center;

    int16_t gyro_x; // follows right-hand-rules
    int16_t gyro_y; // follows right-hand-rules
    int16_t gyro_z; // follows right-hand-rules

    int16_t accel_x; // positive: right
    int16_t accel_y; // positive: up
    int16_t accel_z; // positive: towards player


    //uint8_t 

} gamepad_status_t;

#define LOGIC_FLAGS_VIRT_DS4_ENABLE   0x00000001U
#define LOGIC_FLAGS_PLATFORM_ENABLE   0x00000002U

typedef struct logic {

    rc71l_platform_t platform;

    pthread_mutex_t gamepad_mutex;
    gamepad_status_t gamepad;

    queue_t input_queue;

    pthread_t virt_ds4_thread;

    volatile uint32_t flags;

} logic_t;

int logic_create(logic_t *const logic);

int is_rc71l_ready(const logic_t *const logic);

int logic_copy_gamepad_status(logic_t *const logic, gamepad_status_t *const out);

int logic_begin_status_update(logic_t *const logic);

void logic_end_status_update(logic_t *const logic);