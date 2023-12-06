#pragma once

#include "rogue_enemy.h"

#define IMU_MESSAGE_FLAGS_ACCEL   0x00000001U
#define IMU_MESSAGE_FLAGS_ANGLVEL 0x00000002U

typedef struct imu_message {
    struct timeval gyro_read_time;

    long gyro_x_raw;
    long gyro_y_raw;
    long gyro_z_raw;

    struct timeval accel_read_time;

    long accel_x_raw;
    long accel_y_raw;
    long accel_z_raw;

    int16_t temp_raw;
    double temp_in_k;

    double gyro_rad_s[3]; // | x, y, z| right-hand-rules -- in rad/s
    double accel_m2s[3]; // | x, y, z| positive: right, up, towards player -- in m/s^2

    uint32_t flags;

} imu_in_message_t;
