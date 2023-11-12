#include "dev_iio.h"
#include <stdlib.h>

static char* read_file(const char* base_path, const char *file) {
    char* fdir = NULL;
    long len = 0;

    len = strlen(base_path) + strlen(file) + 1;
    fdir = malloc(len);
    if (fdir == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device path, device skipped.\n", len);
        goto read_file_err;
    }
    strcpy(fdir, base_path);
    strcat(fdir, file);

    if (access(fdir, F_OK) == 0) {
        FILE* fp = fopen(fdir, "r");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            len = ftell(fp);
            rewind(fp);

            free(fdir);
            fdir = NULL;

            len += 1;
            char* res = malloc(len);
            if (res != NULL) {
                fread(res, 1, len, fp);
                fclose(fp);

                return res;
            } else {
                fprintf(stderr, "Cannot allocate %ld bytes for %s content.\n", len, fdir);
                goto read_file_err;
            }
        } else {
            fprintf(stderr, "Cannot open file %s.\n", fdir);
            free(fdir);
            goto read_file_err;
        }
    } else {
        fprintf(stderr, "File %s does not exists.\n", fdir);
        free(fdir);
        goto read_file_err;
    }

read_file_err:
    return NULL;
}

dev_iio_t* dev_iio_create(const char* path) {
    dev_iio_t *iio = malloc(sizeof(dev_iio_t));
    iio->anglvel_x_fd = NULL;
    iio->anglvel_y_fd = NULL;
    iio->anglvel_z_fd = NULL;
    iio->accel_x_fd = NULL;
    iio->accel_y_fd = NULL;
    iio->accel_z_fd = NULL;

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

    const long path_len = strlen(path) + 1;
    iio->path = malloc(path_len);
    if (iio->path == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device name, device skipped.\n", path_len);
        free(iio);
        iio = NULL;
        goto dev_iio_create_err;
    }
    strcpy(iio->path, path);

    // ============================================= DEVICE NAME ================================================
    iio->name = read_file(iio->path, "/name");
    if (iio->name == NULL) {
        fprintf(stderr, "Unable to read iio device name.\n");
        free(iio);
        iio = NULL;
        goto dev_iio_create_err;
    } else {
        int idx = strlen(iio->name) - 1;
        if ((iio->name[idx] == '\n') || ((iio->name[idx] == '\t'))) {
            iio->name[idx] = '\0';
        }
    }
    // ==========================================================================================================

    // ========================================== in_anglvel_scale ==============================================
    {
        char* const anglvel_scale = read_file(iio->path, "/in_anglvel_scale");
        if (anglvel_scale != NULL) {
            iio->anglvel_scale_x = iio->anglvel_scale_y = iio->anglvel_scale_z = strtod(anglvel_scale, NULL);
            free((void*)anglvel_scale);
        } else {
            // TODO: what about if those are plit in in_anglvel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_anglvel_scale from path %s%s.\n", iio->path, "/in_accel_scale");

            free(iio);
            iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // =========================================== in_accel_scale ===============================================
    {
        char* const accel_scale = read_file(iio->path, "/in_accel_scale");
        if (accel_scale != NULL) {
            iio->accel_scale_x = iio->accel_scale_y = iio->accel_scale_z = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            // TODO: what about if those are plit in in_accel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", iio->path, "/in_accel_scale");

            free(iio);
            iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    const size_t tmp_sz = path_len + 128 + 1;
    char* const tmp = malloc(tmp_sz);

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_x_raw");
    iio->accel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_y_raw");
    iio->accel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_accel_z_raw");
    iio->accel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_x_raw");
    iio->anglvel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_y_raw");
    iio->anglvel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_z_raw");
    iio->anglvel_z_fd = fopen(tmp, "r");

    free(tmp);

dev_iio_create_err:
    return iio;
}

void dev_iio_destroy(dev_iio_t* iio) {
    fclose(iio->accel_x_fd);
    fclose(iio->accel_y_fd);
    fclose(iio->accel_z_fd);
    fclose(iio->anglvel_x_fd);
    fclose(iio->anglvel_y_fd);
    fclose(iio->anglvel_z_fd);
    free(iio->name);
    free(iio->path);
    free(iio);
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
    } else if (iio->accel_x_fd != NULL) {
        rewind(iio->accel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_x_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_x;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_X;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_x);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->accel_y_fd != NULL) {
        rewind(iio->accel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_y_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_y;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_Y;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_y);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->accel_z_fd != NULL) {
        rewind(iio->accel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_z_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->accel_scale_z;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_Z;
            ev->value = (__s32)(val_in_m2s * iio->outer_accel_scale_z);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_x_fd != NULL) {
        rewind(iio->anglvel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_x_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_x;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RX;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_x);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_y_fd != NULL) {
        rewind(iio->anglvel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_y_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_y;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RY;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_y);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    if (*buf_out == buf_sz) {
        return -ENOMEM;
    } else if (iio->anglvel_z_fd != NULL) {
        rewind(iio->anglvel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_z_fd);
        if (tmp_read >= 0) {
            gettimeofday(&now, NULL);
            const double val = strtod(&tmp[0], NULL);
            const double val_in_m2s = val * iio->anglvel_scale_z;

            struct input_event* ev = &buf[*buf_out];

            ev->time = now;
            ev->type = EV_ABS;
            ev->code = ABS_RZ;
            ev->value = (__s32)(val_in_m2s * iio->outer_anglvel_scale_z);
        } else {
            return tmp_read;
        }

        ++(*buf_out);
    }

    return 0;
}
