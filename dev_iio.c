#include "dev_iio.h"
#include <stdlib.h>

dev_iio_t* dev_iio_create(const char* path) {
    dev_iio_t *const iio = malloc(sizeof(dev_iio_t));
    iio->anglvel_x = -1;
    iio->anglvel_y = -1;
    iio->anglvel_z = -1;
    iio->accel_x = -1;
    iio->accel_y = -1;
    iio->accel_z = -1;

    iio->accel_scale_x = 0.0f;
    iio->accel_scale_y = 0.0f;
    iio->accel_scale_z = 0.0f;
    iio->anglvel_scale_x = 0.0f;
    iio->anglvel_scale_y = 0.0f;
    iio->anglvel_scale_z = 0.0f;

    iio->outer_accel_scale_x = ACCEL_SCALE;
    iio->outer_accel_scale_y = ACCEL_SCALE;
    iio->outer_accel_scale_z = ACCEL_SCALE;
    iio->outer_anglvel_scale_x = GYRO_SCALE;
    iio->outer_anglvel_scale_y = GYRO_SCALE;
    iio->outer_anglvel_scale_z = GYRO_SCALE;

    return iio;
}

void dev_iio_destroy(dev_iio_t* iio) {
    free(iio->name);
    free(iio->path);
    free(iio->accel_scale_x_fd);
    free(iio->accel_scale_y_fd);
    free(iio->accel_scale_z_fd);
    free(iio->anglvel_scale_x_fd);
    free(iio->anglvel_scale_y_fd);
    free(iio->anglvel_scale_z_fd);
}

const char* dev_iio_get_name(const dev_iio_t* iio) {
    return iio->name;
}

const char* dev_iio_get_path(const dev_iio_t* iio) {
    return iio->path;
}

int dev_iio_read(
    const dev_iio_t *const iio,
    struct input_event *const buf,
    size_t buf_sz,
    uint32_t *const buf_out
) {
    *buf_out = 0;
    char tmp[128];

    struct timeval now = {0};

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->accel_x != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->accel_x, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_x;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_X;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_x);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->accel_y != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->accel_y, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_y;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_Y;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_y);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->accel_z != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->accel_z, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_z;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_Z;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_z);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_x != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->anglvel_x, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_x;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RX;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_x);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_y != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->anglvel_y, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_y;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RY;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_y);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_z != -1) {
        memset((void*)&tmp[0], 0, sizeof(tmp));
        int tmp_read = read(iio->anglvel_z, (void*)&tmp[0], sizeof(tmp));
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_z;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RZ;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_z);
        
            // TODO: seek
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    return 0;
}
