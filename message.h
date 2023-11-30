#pragma once

#include "imu_message.h"

#define MESSAGE_FLAGS_HANDLE_DONE         0x00000001U
#define EV_MESSAGE_FLAGS_PRESERVE_TIME    0x00000002U
#define EV_MESSAGE_FLAGS_IMU              0x00000004U
#define EV_MESSAGE_FLAGS_MOUSE            0x00000008U

typedef struct ev_message {
    struct input_event* ev;
    
    uint32_t ev_flags;

    uint32_t ev_count;
    
    size_t ev_size;

} ev_message_t;
#define HIDRAW_DATA_SIZE 64
typedef struct hidraw_message {
    unsigned char data[HIDRAW_DATA_SIZE];
    ssize_t data_size;
} hidraw_message_t;

typedef enum message_type {
    MSG_TYPE_EV = 0,
    MSG_TYPE_IMU,
    MSG_TYPE_HIDRAW,
} message_type_t;

#define INPUT_FILTER_FLAGS_NONE             0x00000000U
#define INPUT_FILTER_FLAGS_DO_NOT_EMIT      0x00000001U

typedef struct message {
    message_type_t type;

    union {
        imu_message_t imu;
        ev_message_t event;
        hidraw_message_t hidraw;
    } data;

    volatile uint32_t flags;
} message_t;
