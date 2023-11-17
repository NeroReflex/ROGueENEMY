#pragma once

#include "rogue_enemy.h"

typedef struct imu_message {
    double gyro_x_in_rad_s;
    long gyro_x_raw;
    
    double gyro_y_in_rad_s;
    int16_t gyro_y_raw;
    
    double gyro_z_in_rad_s;
    int16_t gyro_z_raw;

    double accel_x_in_m2s;
    int16_t accel_x_raw;
    
    double accel_y_in_m2s;
    int16_t accel_y_raw;

    double accel_z_in_m2s;
    int16_t accel_z_raw;
} imu_message_t;