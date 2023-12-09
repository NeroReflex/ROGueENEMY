#include "dev_iio.h"
#include <stdlib.h>

#define MAX_PATH_LEN 512

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

static int dev_iio_create(const char* path, dev_iio_t **const out_iio) {
    int res = -ENOENT;

    *out_iio = malloc(sizeof(dev_iio_t));
    if (*out_iio == NULL) {
        fprintf(stderr, "Cannot allocate memory for iio device\n");
        res = -ENOMEM;
        goto dev_iio_create_err;
    }

    (*out_iio)->flags = 0x00000000U;
    (*out_iio)->anglvel_x_fd = NULL;
    (*out_iio)->anglvel_y_fd = NULL;
    (*out_iio)->anglvel_z_fd = NULL;
    (*out_iio)->accel_x_fd = NULL;
    (*out_iio)->accel_y_fd = NULL;
    (*out_iio)->accel_z_fd = NULL;
    (*out_iio)->temp_fd = NULL;

    (*out_iio)->accel_scale_x = 0.0f;
    (*out_iio)->accel_scale_y = 0.0f;
    (*out_iio)->accel_scale_z = 0.0f;
    (*out_iio)->anglvel_scale_x = 0.0f;
    (*out_iio)->anglvel_scale_y = 0.0f;
    (*out_iio)->anglvel_scale_z = 0.0f;
    (*out_iio)->temp_scale = 0.0f;

    (*out_iio)->outer_accel_scale_x = ACCEL_SCALE;
    (*out_iio)->outer_accel_scale_y = ACCEL_SCALE;
    (*out_iio)->outer_accel_scale_z = ACCEL_SCALE;
    (*out_iio)->outer_anglvel_scale_x = GYRO_SCALE;
    (*out_iio)->outer_anglvel_scale_y = GYRO_SCALE;
    (*out_iio)->outer_anglvel_scale_z = GYRO_SCALE;
    (*out_iio)->outer_temp_scale = 0.0;

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
    memcpy((*out_iio)->mount_matrix, mm, sizeof(mm));

    const long path_len = strlen(path) + 1;
    (*out_iio)->path = malloc(path_len);
    if ((*out_iio)->path == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device name, device skipped.\n", path_len);
        free(*out_iio);
        *out_iio = NULL;
        goto dev_iio_create_err;
    }
    strcpy((*out_iio)->path, path);

    // ============================================= DEVICE NAME ================================================
    (*out_iio)->name = read_file((*out_iio)->path, "/name");
    if ((*out_iio)->name == NULL) {
        fprintf(stderr, "Unable to read iio device name.\n");
        free(*out_iio);
        *out_iio = NULL;
        goto dev_iio_create_err;
    } else {
        int idx = strlen((*out_iio)->name) - 1;
        if (((*out_iio)->name[idx] == '\n') || (((*out_iio)->name[idx] == '\t'))) {
            (*out_iio)->name[idx] = '\0';
        }
    }
    // ==========================================================================================================

    // ========================================== in_anglvel_scale ==============================================
    {
        const char* preferred_scale = LSB_PER_RAD_S_2000_DEG_S_STR;
        const char *scale_main_file = "/in_anglvel_scale";

        char* const anglvel_scale = read_file((*out_iio)->path, scale_main_file);
        if (anglvel_scale != NULL) {
            (*out_iio)->flags |= DEV_IIO_HAS_ANGLVEL;
            (*out_iio)->anglvel_scale_x = (*out_iio)->anglvel_scale_y = (*out_iio)->anglvel_scale_z = strtod(anglvel_scale, NULL);
            free((void*)anglvel_scale);

            if (write_file((*out_iio)->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                (*out_iio)->anglvel_scale_x = (*out_iio)->anglvel_scale_y = (*out_iio)->anglvel_scale_z = LSB_PER_RAD_S_2000_DEG_S;
                printf("anglvel scale changed to %f for device %s\n", (*out_iio)->anglvel_scale_x, (*out_iio)->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_anglvel_scale for device %s.\n", (*out_iio)->name);
            }
        } else {
            // TODO: what about if those are split in in_anglvel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_anglvel_scale from path %s%s.\n", (*out_iio)->path, scale_main_file);

            free(*out_iio);
            *out_iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // =========================================== in_accel_scale ===============================================
    {
        const char* preferred_scale = LSB_PER_16G_STR;
        const char *scale_main_file = "/in_accel_scale";

        char* const accel_scale = read_file((*out_iio)->path, scale_main_file);
        if (accel_scale != NULL) {
            (*out_iio)->flags |= DEV_IIO_HAS_ACCEL;
            (*out_iio)->accel_scale_x = (*out_iio)->accel_scale_y = (*out_iio)->accel_scale_z = strtod(accel_scale, NULL);
            free((void*)accel_scale);

            if (write_file((*out_iio)->path, scale_main_file, preferred_scale, strlen(preferred_scale)) >= 0) {
                (*out_iio)->accel_scale_x = (*out_iio)->accel_scale_y = (*out_iio)->accel_scale_z = LSB_PER_16G;
                printf("accel scale changed to %f for device %s\n", (*out_iio)->accel_scale_x, (*out_iio)->name);
            } else {
                fprintf(stderr, "Unable to set preferred in_accel_scale for device %s.\n", (*out_iio)->name);
            }
        } else {
            // TODO: what about if those are plit in in_accel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", (*out_iio)->path, scale_main_file);

            free(*out_iio);
            *out_iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================= temp_scale =================================================
    {
        const char *scale_main_file = "/in_temp_scale";

        char* const accel_scale = read_file((*out_iio)->path, scale_main_file);
        if (accel_scale != NULL) {
            (*out_iio)->temp_scale = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            fprintf(stderr, "Unable to read in_temp_scale file from path %s%s.\n", (*out_iio)->path, scale_main_file);

            free(*out_iio);
            *out_iio = NULL;
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================ samplig_freq ================================================
    {
        const char* const preferred_samplig_freq = " 1600.000000";
        const size_t preferred_samplig_freq_len = strlen(preferred_samplig_freq);

        if (write_file((*out_iio)->path, "/in_accel_sampling_frequency", preferred_samplig_freq, preferred_samplig_freq_len) >= 0) {
            printf("Accel sampling frequency changed to %s\n", preferred_samplig_freq);
        } else {
            fprintf(stderr, "Could not change accel sampling frequency\n");
        }

        if (write_file((*out_iio)->path, "/in_anglvel_sampling_frequency", preferred_samplig_freq, preferred_samplig_freq_len) >= 0) {
            printf("Gyro sampling frequency changed to %s\n", preferred_samplig_freq);
        } else {
            fprintf(stderr, "Could not change gyro sampling frequency\n");
        }
    }
    // ==========================================================================================================

    const size_t tmp_sz = path_len + 128 + 1;
    char* const tmp = malloc(tmp_sz);

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_accel_x_raw");
    (*out_iio)->accel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_accel_y_raw");
    (*out_iio)->accel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_accel_z_raw");
    (*out_iio)->accel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_anglvel_x_raw");
    (*out_iio)->anglvel_x_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_anglvel_y_raw");
    (*out_iio)->anglvel_y_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_anglvel_z_raw");
    (*out_iio)->anglvel_z_fd = fopen(tmp, "r");

    memset(tmp, 0, tmp_sz);
    strcat(tmp, (*out_iio)->path);
    strcat(tmp, "/in_temp_raw");
    (*out_iio)->temp_fd = fopen(tmp, "r");

    free(tmp);

/*
    printf(
        "anglvel scale: x=%f, y=%f, z=%f | accel scale: x=%f, y=%f, z=%f\n",
        (*out_iio)->anglvel_scale_x,
        (*out_iio)->anglvel_scale_y,
        (*out_iio)->anglvel_scale_z,
        (*out_iio)->accel_scale_x,
        (*out_iio)->accel_scale_y,
        (*out_iio)->accel_scale_z
    );
*/

    res = 0;

dev_iio_create_err:
    return res;
}

void dev_iio_close(dev_iio_t* iio) {
    if (iio == NULL) {
        return;
    }

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

static bool iio_matches(
    const iio_filters_t *const in_filters,
    dev_iio_t *const in_dev
) {
    if (in_dev == NULL) {
        return false;
    }

    const char *const name = dev_iio_get_name(in_dev);
    if ((name == NULL) || (strcmp(name, in_filters->name) != 0)) {
        return false;
    }

    return true;
}

/*
modprobe industrialio-sw-trigger
modprobe iio-trig-sysfs
modprobe iio-trig-hrtimer

# sysfs-trigger
echo 1 > /sys/bus/iio/devices/iio_sysfs_trigger/add_trigger
cat  /sys/bus/iio/devices/trigger0/name # I got sysfstrig1
cd /sys/bus/iio/devices
cd iio\:device0
echo "sysfstrig1" > trigger/current_trigger
echo 1 > buffer0/in_anglvel_x_en
echo 1 > buffer0/in_anglvel_y_en
echo 1 > buffer0/in_anglvel_z_en
echo 1 > buffer0/in_accel_x_en
echo 1 > buffer0/in_accel_x_en
echo 1 > buffer0/in_accel_x_en
echo 1 > buffer0/enable
echo 1 > buffer/enable
cd ..
cd trigger0
echo 1 > trigger_now

# hrtimer
mkdir /config/iio/triggers/hrtimer/rogue
mount -t configfs none /config
mkdir /config
*/
static const char *const iio_hrtrigger_name = "iio-trig-hrtimer";

static const char *const iio_path = "/sys/bus/iio/devices/";

int dev_iio_open(
    const iio_filters_t *const in_filters,
    dev_iio_t **const out_dev
) {
    int res = -ENOENT;

    char path[MAX_PATH_LEN] = "\0";
    
    DIR *d;
    struct dirent *dir;
    d = opendir(iio_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') {
                continue;
            }

            snprintf(path, MAX_PATH_LEN - 1, "%s%s", iio_path, dir->d_name);

            //printf("Testing for device %s\n", path);

            // try to open the device, if it cannot be opened to go the next
            const int iio_creation_res = dev_iio_create(path, out_dev);
            if (iio_creation_res != 0) {
                //fprintf(stderr, "Cannot open %s, device skipped.\n", path);
                continue;
            }

            if (!iio_matches(in_filters, *out_dev)) {
                dev_iio_close(*out_dev);
                continue;
            }

            // the device has been found
            res = 0;
            break;
        }
        closedir(d);
    }

/*
    // Load the kernel module
    int result = syscall(__NR_finit_module, -1, iio_hrtrigger_name, 0);

    if (result == 0) {
        printf("Kernel module '%s' loaded successfully.\n", iio_hrtrigger_name);
    } else {
        perror("Error loading kernel module");
    }
*/

dev_iio_open_err:
    return res;
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

int dev_iio_read_imu(const dev_iio_t *const iio, imu_in_message_t *const out) {
    struct timeval read_time;
    gettimeofday(&read_time, NULL);

    out->flags = 0x00000000U;

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
            if ((out->flags & IMU_MESSAGE_FLAGS_ACCEL) == 0) {
                out->accel_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ACCEL;
            }
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
            if ((out->flags & IMU_MESSAGE_FLAGS_ACCEL) == 0) {
                out->accel_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ACCEL;
            }
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
            if ((out->flags & IMU_MESSAGE_FLAGS_ACCEL) == 0) {
                out->accel_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ACCEL;
            }
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
            if ((out->flags & IMU_MESSAGE_FLAGS_ANGLVEL) == 0) {
                out->gyro_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ANGLVEL;
            }
        } else {
            fprintf(stderr, "While reading anglvel(x): %d\n", tmp_read);
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
            if ((out->flags & IMU_MESSAGE_FLAGS_ANGLVEL) == 0) {
                out->gyro_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ANGLVEL;
            }
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
            if ((out->flags & IMU_MESSAGE_FLAGS_ANGLVEL) == 0) {
                out->gyro_read_time = read_time;
                out->flags |= IMU_MESSAGE_FLAGS_ANGLVEL;
            }
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
