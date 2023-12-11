#pragma once

#include "imu_message.h"

#include "input_dev.h"

#define DEV_IIO_HAS_ACCEL   0x00000001U
#define DEV_IIO_HAS_ANGLVEL 0x00000002U

#define ACCEL_SCALE     ((double)(255.0)/(double)(9.81)) // convert m/s^2 to g's, and scale x255 to increase precision when passed to evdev as an int
#define GYRO_SCALE      ((double)(180.0)/(double)(M_PI))  // convert radians/s to degrees/s

typedef struct dev_iio {
    char* path;
    char* dev_path;
    char* name;
    uint32_t flags;
    int fd;

    double accel_scale_x;
    double accel_scale_y;
    double accel_scale_z;

    double outer_accel_scale_x;
    double outer_accel_scale_y;
    double outer_accel_scale_z;

    double anglvel_scale_x;
    double anglvel_scale_y;
    double anglvel_scale_z;

    double outer_anglvel_scale_x;
    double outer_anglvel_scale_y;
    double outer_anglvel_scale_z;

    double temp_scale;
    
    double outer_temp_scale;
} dev_iio_t;

int dev_iio_open(
    const iio_filters_t *const in_filters,
    dev_iio_t **const out_dev
);

void dev_iio_close(dev_iio_t *const iio);

int dev_iio_get_buffer_fd(const dev_iio_t *const iio);

const char* dev_iio_get_name(const dev_iio_t* iio);

const char* dev_iio_get_path(const dev_iio_t* iio);

int dev_iio_has_anglvel(const dev_iio_t* iio);

int dev_iio_has_accel(const dev_iio_t* iio);

int dev_iio_change_anglvel_sampling_freq(const dev_iio_t *const iio, const char *const freq_str_hz);

int dev_iio_change_accel_sampling_freq(const dev_iio_t *const iio, const char *const freq_str_hz);
