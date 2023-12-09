#pragma once

#include "imu_message.h"

#define EV_MESSAGE_FLAGS_PRESERVE_TIME    0x00000001U
#define EV_MESSAGE_FLAGS_IMU              0x00000002U
#define EV_MESSAGE_FLAGS_MOUSE            0x00000004U

typedef enum in_message_gamepad_btn {
    GAMEPAD_BTN_CROSS,
    GAMEPAD_BTN_CIRCLE,
    GAMEPAD_BTN_SQUARE,
    GAMEPAD_BTN_TRIANGLE,
    GAMEPAD_BTN_OPTION,
    GAMEPAD_BTN_SHARE,
    GAMEPAD_BTN_L1,
    GAMEPAD_BTN_R1,
    GAMEPAD_BTN_L2_TRIGGER,
    GAMEPAD_BTN_R2_TRIGGER,
    GAMEPAD_BTN_L3,
    GAMEPAD_BTN_R3,
    GAMEPAD_BTN_L4,
    GAMEPAD_BTN_R4,
    GAMEPAD_BTN_L5,
    GAMEPAD_BTN_R5,
    GAMEPAD_BTN_TOUCHPAD,

    GAMEPAD_LEFT_JOYSTICK_X,
    GAMEPAD_LEFT_JOYSTICK_Y,
    GAMEPAD_RIGHT_JOYSTICK_X,
    GAMEPAD_RIGHT_JOYSTICK_Y,

    GAMEPAD_DPAD_X,
    GAMEPAD_DPAD_Y,
}  in_gamepad_element_t;

typedef struct in_message_gamepad_set_element {
    in_gamepad_element_t element;
    union {
        uint8_t btn;
        int32_t joystick_pos;
        int8_t dpad; // -1 | 0 | +1
    } status;
}  in_message_gamepad_set_element_t;

typedef enum in_message_gamepad_action {
    GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER,
    GAMEPAD_ACTION_OPEN_STEAM_QAM,
} in_message_gamepad_action_t;

typedef enum in_in_message_type {
    GAMEPAD_SET_ELEMENT,
    GAMEPAD_ACTION,
}  in_message_type_t;

typedef struct in_message {
    in_message_type_t type;

    union {
        //imu_in_message_t imu;

        in_message_gamepad_action_t action;

        in_message_gamepad_set_element_t gamepad_set;
    } data;

}  in_message_t;


typedef struct out_message_rumble {
    uint8_t motors_left;
    uint8_t motors_right;
}  out_message_rumble_t;

typedef struct out_message_leds {
    uint8_t r;
    uint8_t g;
    uint8_t b;
}  out_message_leds_t;

typedef enum out_message_type {
    OUT_MSG_TYPE_RUMBLE = 0,
    OUT_MSG_TYPE_LEDS,
}  out_message_type_t;

typedef struct out_message {
    out_message_type_t type;

    union {
        out_message_rumble_t rumble;
        out_message_leds_t leds;
    } data;

}  out_message_t;
