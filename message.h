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

    GAMEPAD_GYROSCOPE,
    GAMEPAD_ACCELEROMETER,
}  in_gamepad_element_t;

typedef struct in_message_gamepad_gyro {
    struct timeval sample_time;

    uint16_t x;
    uint16_t y;
    uint16_t z;
} in_message_gamepad_gyro_t;

typedef struct in_message_gamepad_accel {
    struct timeval sample_time;

    uint16_t x;
    uint16_t y;
    uint16_t z;
} in_message_gamepad_accel_t;

typedef struct in_message_gamepad_set_element {
    in_gamepad_element_t element;
    union {
        uint8_t btn;
        int32_t joystick_pos;
        int8_t dpad; // -1 | 0 | +1
        in_message_gamepad_accel_t accel;
        in_message_gamepad_gyro_t gyro;
    } status;
}  in_message_gamepad_set_element_t;

typedef enum mouse_element {
    MOUSE_ELEMENT_X,
    MOUSE_ELEMENT_Y,
    MOUSE_BTN_LEFT,
    MOUSE_BTN_MIDDLE,
    MOUSE_BTN_RIGHT,
} mouse_element_t;

typedef struct in_message_mouse_event {
    mouse_element_t type;
    int32_t value;
} in_message_mouse_event_t;

typedef enum in_message_gamepad_action {
    GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER,
    GAMEPAD_ACTION_OPEN_STEAM_QAM,
} in_message_gamepad_action_t;

typedef enum kbd_element {
    KEYBOARD_KEY_Q,
    KEYBOARD_KEY_W,
    KEYBOARD_KEY_E,
    KEYBOARD_KEY_R,
    KEYBOARD_KEY_T,
    KEYBOARD_KEY_Y,
    KEYBOARD_KEY_U,
    KEYBOARD_KEY_I,
    KEYBOARD_KEY_O,
    KEYBOARD_KEY_P,
    KEYBOARD_KEY_A,
    KEYBOARD_KEY_S,
    KEYBOARD_KEY_D,
    KEYBOARD_KEY_F,
    KEYBOARD_KEY_G,
    KEYBOARD_KEY_H,
    KEYBOARD_KEY_J,
    KEYBOARD_KEY_K,
    KEYBOARD_KEY_L,
    KEYBOARD_KEY_Z,
    KEYBOARD_KEY_X,
    KEYBOARD_KEY_C,
    KEYBOARD_KEY_V,
    KEYBOARD_KEY_B,
    KEYBOARD_KEY_N,
    KEYBOARD_KEY_M,
    KEYBOARD_KEY_UP,
    KEYBOARD_KEY_DOWN,
    KEYBOARD_KEY_LEFT,
    KEYBOARD_KEY_RIGHT,
    KEYBOARD_KEY_NUM_1,
    KEYBOARD_KEY_NUM_2,
    KEYBOARD_KEY_NUM_3,
    KEYBOARD_KEY_NUM_4,
    KEYBOARD_KEY_NUM_5,
    KEYBOARD_KEY_NUM_6,
    KEYBOARD_KEY_NUM_7,
    KEYBOARD_KEY_NUM_8,
    KEYBOARD_KEY_NUM_9,
    KEYBOARD_KEY_NUM_0,
    KEYBOARD_KEY_LCRTL,
} kbd_element_t;

typedef struct in_message_keyboard_set_element {
    kbd_element_t type;
    uint8_t value;
} in_message_keyboard_set_element_t;

typedef enum in_in_message_type {
    GAMEPAD_SET_ELEMENT,
    GAMEPAD_ACTION,
    MOUSE_EVENT,
    KEYBOARD_SET_ELEMENT,
}  in_message_type_t;

typedef struct in_message {
    in_message_type_t type;

    union {
        in_message_gamepad_action_t action;

        in_message_gamepad_set_element_t gamepad_set;

        in_message_mouse_event_t mouse_event;

        in_message_keyboard_set_element_t kbd_set;
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
