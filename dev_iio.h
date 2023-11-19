#pragma once

#include "imu_message.h"

#define DEV_IIO_HAS_ACCEL   0x00000001U
#define DEV_IIO_HAS_ANGLVEL 0x00000002U

#define ACCEL_SCALE     ((double)(255.0)/(double)(9.81)) // convert m/s^2 to g's, and scale x255 to increase precision when passed to evdev as an int
#define GYRO_SCALE      ((double)(180.0)/(double)(M_PI))  // convert radians/s to degrees/s

typedef struct dev_iio {
    char* path;
    char* name;
    uint32_t flags;

    FILE* accel_x_fd;
    FILE* accel_y_fd;
    FILE* accel_z_fd;

    double accel_scale_x;
    double accel_scale_y;
    double accel_scale_z;

    double outer_accel_scale_x;
    double outer_accel_scale_y;
    double outer_accel_scale_z;

    FILE* anglvel_x_fd;
    FILE* anglvel_y_fd;
    FILE* anglvel_z_fd;

    double anglvel_scale_x;
    double anglvel_scale_y;
    double anglvel_scale_z;

    double outer_anglvel_scale_x;
    double outer_anglvel_scale_y;
    double outer_anglvel_scale_z;

    FILE* temp_fd;

    double temp_scale;
    
    double outer_temp_scale;

    double mount_matrix[3][3];

    double sampling_rate_hz;
} dev_iio_t;

dev_iio_t* dev_iio_create(const char* path);

void dev_iio_destroy(dev_iio_t* iio);

const char* dev_iio_get_name(const dev_iio_t* iio);

const char* dev_iio_get_path(const dev_iio_t* iio);

inline int dev_iio_has_anglvel(const dev_iio_t* iio) {
    return (iio->flags & DEV_IIO_HAS_ANGLVEL) != 0;
}

inline int dev_iio_has_accel(const dev_iio_t* iio) {
    return (iio->flags & DEV_IIO_HAS_ACCEL) != 0;
}

int dev_iio_read(
    const dev_iio_t *const iio,
    struct input_event *const buf,
    size_t buf_sz,
    uint32_t *const buf_out
);

int dev_iio_read_imu(
    const dev_iio_t *const iio,
    imu_message_t *const out
);