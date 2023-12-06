#pragma once

#include "imu_message.h"

#define EV_MESSAGE_FLAGS_PRESERVE_TIME    0x00000001U
#define EV_MESSAGE_FLAGS_IMU              0x00000002U
#define EV_MESSAGE_FLAGS_MOUSE            0x00000004U

#define MAX_EVDEV_EVENTS_IN_MESSAGE 16

typedef enum in_message_gamepad_btn {
    GAMEPAD_BTN_A,
    GAMEPAD_BTN_B,
    GAMEPAD_BTN_X,
    GAMEPAD_BTN_Y,
    GAMEPAD_BTN_START,
    GAMEPAD_BTN_SELECT,
    GAMEPAD_BTN_L1,
    GAMEPAD_BTN_R1,
    GAMEPAD_BTN_L2,
    GAMEPAD_BTN_R2,
    GAMEPAD_BTN_L3,
    GAMEPAD_BTN_R3,
} __packed in_message_gamepad_btn_t;

typedef struct in_message_gamepad_btn_change {
    in_message_gamepad_btn_t button;
    uint8_t status;
} __packed in_message_gamepad_btn_change_t;

typedef enum in_message_gamepad_delta_type {
    GAMEPAD_DELTA_BTN,
} __packed in_message_gamepad_delta_type_t;

typedef struct in_message_gamepad_delta {
    in_message_gamepad_delta_type_t type;

    union {
        in_message_gamepad_btn_change_t btn;
    } data;
} __packed in_message_gamepad_delta_t;

typedef enum in_in_message_type {
    IN_MSG_TYPE_BTN,
    IN_MSG_TYPE_SENSOR,
    IN_MSG_TYPE_MACRO,
} __packed in_message_type_t;

typedef struct in_message {
    in_message_type_t type;

    union {
        //imu_in_message_t imu;

        in_message_gamepad_delta_t gamepad_delta;
    } data;

} __packed in_message_t;


typedef struct out_message_rumble {
    uint16_t strong_magnitude;
    uint16_t weak_magnitude;
} __packed out_message_rumble_t;

typedef struct out_message_leds {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __packed out_message_leds_t;

typedef enum out_message_type {
    OUT_MSG_TYPE_RUMBLE = 0,
    OUT_MSG_TYPE_LEDS,
} __packed out_message_type_t;

typedef struct out_message {
    out_message_type_t type;

    union {
        out_message_rumble_t rumble;
        out_message_leds_t leds;
    } data;

} __packed out_message_t;
