#include "dev_iio.h"
#include <stdlib.h>

static char* read_file(const char* base_path, const char *file) {
    char* res = NULL;
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

            len += 1;
            res = malloc(len);
            if (res != NULL) {
                unsigned long read_bytes = fread(res, 1, len, fp);
                printf("Read %lu bytes from file %s\n", read_bytes, fdir);
            } else {
                fprintf(stderr, "Cannot allocate %ld bytes for %s content.\n", len, fdir);
            }

            fclose(fp);
        } else {
            fprintf(stderr, "Cannot open file %s.\n", fdir);
        }
    } else {
        fprintf(stderr, "File %s does not exists.\n", fdir);
    }

    free(fdir);
    fdir = NULL;

read_file_err:
    return res;
}

int write_file(const char* base_path, const char *file, const void* buf, size_t buf_sz) {
    char* fdir = NULL;

    int res = 0;

    const size_t len = strlen(base_path) + strlen(file) + 1;
    fdir = malloc(len);
    if (fdir == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device path, device skipped.\n", len);
        goto write_file_err;
    }
    strcpy(fdir, base_path);
    strcat(fdir, file);

    if (access(fdir, F_OK) == 0) {
        FILE* fp = fopen(fdir, "w");
        if (fp != NULL) {
            res = fwrite(buf, 1, buf_sz, fp);
            if (res >= buf_sz) {
                printf("Written %d bytes to file %s\n", res, fdir);
            } else {
                fprintf(stderr, "Cannot write to %s: %d.\n", fdir, res);
            }

            fclose(fp);
        } else {
            fprintf(stderr, "Cannot open file %s.\n", fdir);
        }
    } else {
        fprintf(stderr, "File %s does not exists.\n", fdir);
    }

    free(fdir);
    fdir = NULL;

write_file_err:
    return res;
}

dev_iio_t* dev_iio_create(const char* path) {
    dev_iio_t *iio = malloc(sizeof(dev_iio_t));
    if (iio == NULL) {
        return NULL;
    }

    iio->anglvel_x_fd = NULL;
    iio->anglvel_y_fd = NULL;
    iio->anglvel_z_fd = NULL;
    iio->accel_x_fd = NULL;
    iio->accel_y_fd = NULL;
    iio->accel_z_fd = NULL;
    iio->temp_fd = NULL;

    iio->accel_scale_x = 0.0f;
    iio->accel_scale_y = 0.0f;
    iio->accel_scale_z = 0.0f;
    iio->anglvel_scale_x = 0.0f;
    iio->anglvel_scale_y = 0.0f;
    iio->anglvel_scale_z = 0.0f;
    iio->temp_scale = 0.0f;

    iio->outer_accel_scale_x = ACCEL_SCALE;
    iio->outer_accel_scale_y = ACCEL_SCALE;
    iio->outer_accel_scale_z = ACCEL_SCALE;
    iio->outer_anglvel_scale_x = GYRO_SCALE;
    iio->outer_anglvel_scale_y = GYRO_SCALE;
    iio->outer_anglvel_scale_z = GYRO_SCALE;
    iio->outer_temp_scale = 0.0;

    double mm[3][3] = 
    /*
    // this is the testing but "wrong" mount matrix
    {
        {0.0f, 0.0f, -1.0f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f}
    };
    */
    // this is the correct matrix:
    {
        {-1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, -1.0}
    };

    // store the mount matrix
    memcpy(iio->mount_matrix, mm, sizeof(mm));

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
        const char* preferred_scale = LSB_PER_RAD_S_2000_DEG_S_STR;
        const char *scale_main_file = "/in_anglvel_scale";

        char* const anglvel_scale = read_file(iio->path, scale_main_file);
        if (anglvel_scale != NULL) {
            iio->anglvel_scale_x = iio->anglvel_scale_y = iio->anglvel_scale_z = strtod(anglvel_scale, NULL);
            free((void*)anglvel_scale);

            if (write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->anglvel_scale_x = iio->anglvel_scale_y = iio->anglvel_scale_z = LSB_PER_RAD_S_2000_DEG_S;
                printf("anglvel scale changed to %f for device %s\n", iio->anglvel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_anglvel_scale for device %s.\n", iio->name);
            }
        } else {
            // TODO: what about if those are split in in_anglvel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_anglvel_scale from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // =========================================== in_accel_scale ===============================================
    {
        const char* preferred_scale = LSB_PER_16G_STR;
        // const char *scale_main_file = "/in_accel_scale";
        const char *scale_main_file = "/in_anglvel_scale";


        char* const accel_scale = read_file(iio->path, scale_main_file);
        if (accel_scale != NULL) {
            iio->accel_scale_x = iio->accel_scale_y = iio->accel_scale_z = strtod(accel_scale, NULL);
            free((void*)accel_scale);

            if (write_file(iio->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                iio->accel_scale_x = iio->accel_scale_y = iio->accel_scale_z = LSB_PER_16G;
                printf("accel scale changed to %f for device %s\n", iio->accel_scale_x, iio->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_accel_scale for device %s.\n", iio->name);
            }
        } else {
            // TODO: what about if those are plit in in_accel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", iio->path, scale_main_file);

            free(iio);
            iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================= temp_scale =================================================
    {
        // char* const accel_scale = read_file(iio->path, "/in_temp_scale");
        char* const accel_scale = read_file(iio->path, "/in_anglvel_scale");
        if (accel_scale != NULL) {
            iio->temp_scale = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", iio->path, "/in_accel_scale");

            free(iio);
            iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================ sampling_rate ================================================
    {
        // char* const accel_scale = read_file(iio->path, "/in_temp_scale");
        char* const accel_scale = read_file(iio->path, "/in_anglvel_scale");

        if (accel_scale != NULL) {
            iio->temp_scale = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
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
    // strcat(tmp, "/in_accel_x_raw");
    strcat(tmp, "/in_anglvel_y_raw");
    iio->accel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    // strcat(tmp, "/in_accel_y_raw");
    strcat(tmp, "/in_anglvel_x_raw");

    iio->accel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    // strcat(tmp, "/in_accel_z_raw");
    strcat(tmp, "/in_anglvel_z_raw");
    iio->accel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_y_raw");
    iio->anglvel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_x_raw");
    iio->anglvel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_z_raw");
    iio->anglvel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, iio->path);
    strcat(tmp, "/in_anglvel_z_raw");
    iio->temp_fd = fopen(tmp, "r");

    free(tmp);

    printf(
        "anglvel scale: x=%f, y=%f, z=%f | accel scale: x=%f, y=%f, z=%f\n",
        iio->anglvel_scale_x,
        iio->anglvel_scale_y,
        iio->anglvel_scale_z,
        iio->accel_scale_x,
        iio->accel_scale_y,
        iio->accel_scale_z
    );

    // give time to change the scale
    sleep(4);

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
    fclose(iio->temp_fd);
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
            fprintf(stderr, "While reading accel(x): %d\n", tmp_read);
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
            fprintf(stderr, "While reading accel(y): %d\n", tmp_read);
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
            fprintf(stderr, "While reading accel(z): %d\n", tmp_read);
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
            fprintf(stderr, "While reading anglvel(x): %d\n", tmp_read);
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
            fprintf(stderr, "While reading anglvel(y): %d\n", tmp_read);
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
            fprintf(stderr, "While reading anglvel(z): %d\n", tmp_read);
            return tmp_read;
        }

        ++(*buf_out);
    }

    return 0;
}

static void multiplyMatrixVector(const double matrix[3][3], const double vector[3], double result[3]) {
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2];
}

int dev_iio_read_imu(const dev_iio_t *const iio, imu_message_t *const out) {
    gettimeofday(&out->read_time, NULL);

    char tmp[128];

    double gyro_in[3];
    double accel_in[3];

    double gyro_out[3];
    double accel_out[3];

    if (iio->accel_x_fd != NULL) {
        rewind(iio->accel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_x_fd);
        if (tmp_read >= 0) {
            out->accel_x_raw = strtol(&tmp[0], NULL, 10);
            accel_in[0] = (double)out->accel_x_raw * iio->accel_scale_x;
        } else {
            fprintf(stderr, "While reading accel(x): %d\n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->accel_y_fd != NULL) {
        rewind(iio->accel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_y_fd);
        if (tmp_read >= 0) {
            out->accel_y_raw = strtol(&tmp[0], NULL, 10);
            accel_in[1] = (double)out->accel_y_raw * iio->accel_scale_y;
        } else {
            fprintf(stderr, "While reading accel(y): %d\n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->accel_z_fd != NULL) {
        rewind(iio->accel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->accel_z_fd);
        if (tmp_read >= 0) {
            out->accel_z_raw = strtol(&tmp[0], NULL, 10);
            accel_in[2] = (double)out->accel_z_raw * iio->accel_scale_z;
        } else {
            fprintf(stderr, "While reading accel(z): %d\n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->anglvel_x_fd != NULL) {
        rewind(iio->anglvel_x_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_x_fd);
        if (tmp_read >= 0) {
            out->gyro_x_raw = strtol(&tmp[0], NULL, 10);
            gyro_in[0] = (double)out->gyro_x_raw * iio->anglvel_scale_x;
            printf("X axis: %d\n", gyro_in[0]);
        } else {
            fprintf(stderr, "While reading anglvel(x): %d \n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->anglvel_y_fd != NULL) {
        rewind(iio->anglvel_y_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_y_fd);
        if (tmp_read >= 0) {
            out->gyro_y_raw = strtol(&tmp[0], NULL, 10);
            gyro_in[1] = (double)out->gyro_y_raw *iio->anglvel_scale_y;
            printf("Y axis: %d ", gyro_in[1]);

        } else {
            fprintf(stderr, "While reading anglvel(y): %d\n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->anglvel_z_fd != NULL) {
        rewind(iio->anglvel_z_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->anglvel_z_fd);
        if (tmp_read >= 0) {
            out->gyro_z_raw = strtol(&tmp[0], NULL, 10);
            gyro_in[2] = (double)out->gyro_z_raw *iio->anglvel_scale_z;
            printf("Z axis: %d ", gyro_in[2]);
        } else {
            fprintf(stderr, "While reading anglvel(z): %d\n", tmp_read);
            return tmp_read;
        }
    }

    if (iio->temp_fd != NULL) {
        rewind(iio->temp_fd);
        memset((void*)&tmp[0], 0, sizeof(tmp));
        const int tmp_read = fread((void*)&tmp[0], 1, sizeof(tmp), iio->temp_fd);
        if (tmp_read >= 0) {
            out->temp_raw = strtol(&tmp[0], NULL, 10);
            out->temp_in_k = (double)out->temp_raw *iio->temp_scale;
        } else {
            fprintf(stderr, "While reading temp: %d\n", tmp_read);
            return tmp_read;
        }
    }

    

    multiplyMatrixVector(iio->mount_matrix, gyro_in, gyro_out);
    multiplyMatrixVector(iio->mount_matrix, accel_in, accel_out);

    memcpy(out->accel_m2s, accel_out, sizeof(double[3]));
    memcpy(out->gyro_rad_s, gyro_out, sizeof(double[3]));
    return 0;
}
