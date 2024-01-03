#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

#define GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER  0x00000001U
#define GAMEPAD_STATUS_FLAGS_OPEN_STEAM_QAM             0x00000002U

#define PRESS_AND_RELEASE_DURATION_FOR_CENTER_BUTTON_MS     80
#define PRESS_TIME_BEFORE_CROSS_BUTTON_MS                   250
#define PRESS_TIME_CROSS_BUTTON_MS                          80
#define PRESS_TIME_AFTER_CROSS_BUTTON_MS                    180

typedef struct gamepad_status {
    bool connected;

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

    uint8_t l4;
    uint8_t r4;
    
    uint8_t l5;
    uint8_t r5;

    uint8_t touchpad_press;

    int16_t touchpad_touch_num; // touchpad is inactive when this is -1
    int16_t touchpad_x; // 0 to 1920
    int16_t touchpad_y; // 0 to 1080

    int64_t last_gyro_motion_timestamp_ns;
    int64_t last_accel_motion_timestamp_ns;

    double gyro[3]; // | x, y, z| right-hand-rules -- in rad/s
    double accel[3]; // | x, y, z| positive: right, up, towards player -- in m/s^2

    int16_t raw_gyro[3];
    int16_t raw_accel[3];

    uint64_t rumble_events_count;
    uint8_t motors_intensity[2]; // 0 = left, 1 = right

    uint64_t leds_events_count;
    uint8_t leds_colors[3]; // r | g | b

    uint8_t join_left_analog_and_gyroscope;
    uint8_t join_right_analog_and_gyroscope;

    volatile uint32_t flags;

} gamepad_status_t;

typedef struct keyboard_status {
    bool connected;

    uint8_t q,w,e,r,t,y,u,i,o,p,a,s,d,f,g,h,j,k,l,z,x,c,v,b,n,m;

    uint8_t num_1, num_2, num_3, num_4, num_5, num_6, num_7, num_8, num_9, num_0;

    uint8_t up, down, left, right;

    uint8_t lctrl;

} keyboard_status_t;


typedef struct mouse_status {
    bool connected;

    int32_t x;
    int32_t y;
    
    uint8_t btn_left;
    uint8_t btn_middle;
    uint8_t btn_right;

} mouse_status_t;

typedef struct devices_status {
    // this mutex MUST be grabbed when reading and/or writing below properties
    pthread_mutex_t mutex;

    gamepad_status_t gamepad;

    keyboard_status_t kbd;

    mouse_status_t mouse;

} devices_status_t;

void mouse_status_init(mouse_status_t *const stats);

void kbd_status_init(keyboard_status_t *const stats);

void gamepad_status_init(gamepad_status_t *const stats);

void devices_status_init(devices_status_t *const stats);

void gamepad_status_qam_quirk(gamepad_status_t *const gamepad_stats);

void gamepad_status_qam_quirk_ext_time(gamepad_status_t *const gamepad_stats);
