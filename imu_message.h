#pragma once

#include "rogue_enemy.h"

typedef struct imu_message {
    struct timeval read_time;

    long gyro_x_raw;
    long gyro_y_raw;
    long gyro_z_raw;

    long accel_x_raw;
    long accel_y_raw;
    long accel_z_raw;

    int16_t temp_raw;
    double temp_in_k;

    double gyro_rad_s[3]; // | x, y, z| right-hand-rules -- in rad/s
    double accel_m2s[3]; // | x, y, z| positive: right, up, towards player -- in m/s^2
} imu_message_t;