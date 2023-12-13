#include "dev_iio.h"
#include <stdlib.h>

#define MAX_PATH_LEN 512

struct read_file_res {
    char* buf;
    unsigned long len;
};

static struct read_file_res read_file(const char* base_path, const char *file) {
    struct read_file_res res = {
        .buf = NULL,
        .len = 0,
    };

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
            res.buf = malloc(len);
            if (res.buf != NULL) {
                res.len = fread(res.buf, 1, len, fp);
                //printf("Read %lu bytes from file %s\n", read_bytes, fdir);
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

static int dev_iio_create(char* dev_path, const char* path, dev_iio_t **const out_iio) {
    int res = -ENOENT;

    *out_iio = malloc(sizeof(dev_iio_t));
    if (*out_iio == NULL) {
        fprintf(stderr, "Cannot allocate memory for iio device\n");
        res = -ENOMEM;
        goto dev_iio_create_err;
    }

    (*out_iio)->flags = 0x00000000U;
    (*out_iio)->fd = -1;

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

    const long path_len = strlen(path) + 1;
    (*out_iio)->path = malloc(path_len);
    if ((*out_iio)->path == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device name, device skipped.\n", path_len);
        goto dev_iio_create_err;
    }
    strcpy((*out_iio)->path, path);

    const long dev_path_len = strlen(dev_path) + 1;
    (*out_iio)->dev_path = malloc(dev_path_len);
    if ((*out_iio)->dev_path == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device /dev path, device skipped.\n", path_len);
        goto dev_iio_create_err;
    }
    strcpy((*out_iio)->dev_path, dev_path);

    (*out_iio)->fd = open((*out_iio)->dev_path, O_RDONLY);
    if ((*out_iio)->fd < 0) {
        fprintf(stderr, "Error opening %s: %d\n", (*out_iio)->dev_path, errno);
        res = errno;
        res = res < 0 ? res : -1 * res;
        goto dev_iio_create_err;
    }

    struct read_file_res rr;

    // ============================================= DEVICE NAME ================================================
    rr = read_file((*out_iio)->path, "/name");
    (*out_iio)->name = rr.buf;
    if ((*out_iio)->name == NULL) {
        fprintf(stderr, "Unable to read iio device name.\n");
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
        const char *scale_main_file = "/in_anglvel_scale";
        rr = read_file((*out_iio)->path, scale_main_file);
        char* const anglvel_scale = rr.buf;
        if (anglvel_scale != NULL) {
            (*out_iio)->flags |= DEV_IIO_HAS_ANGLVEL;
            (*out_iio)->anglvel_scale_x = (*out_iio)->anglvel_scale_y = (*out_iio)->anglvel_scale_z = strtod(anglvel_scale, NULL);
            free((void*)anglvel_scale);
        } else {
            // TODO: what about if those are split in in_anglvel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_anglvel_scale from path %s%s.\n", (*out_iio)->path, scale_main_file);
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // =========================================== in_accel_scale ===============================================
    {
        const char *scale_main_file = "/in_accel_scale";
        rr = read_file((*out_iio)->path, scale_main_file);
        char* const accel_scale = rr.buf;
        if (accel_scale != NULL) {
            (*out_iio)->flags |= DEV_IIO_HAS_ACCEL;
            (*out_iio)->accel_scale_x = (*out_iio)->accel_scale_y = (*out_iio)->accel_scale_z = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            // TODO: what about if those are plit in in_accel_{x,y,z}_scale?
            fprintf(stderr, "Unable to read in_accel_scale file from path %s%s.\n", (*out_iio)->path, scale_main_file);
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    // ============================================= temp_scale =================================================
    {
        const char *scale_main_file = "/in_temp_scale";

        rr = read_file((*out_iio)->path, scale_main_file);
        char* const accel_scale = rr.buf;
        if (accel_scale != NULL) {
            (*out_iio)->temp_scale = strtod(accel_scale, NULL);
            free((void*)accel_scale);
        } else {
            fprintf(stderr, "Unable to read in_temp_scale file from path %s%s.\n", (*out_iio)->path, scale_main_file);
            goto dev_iio_create_err;
        }
    }
    // ==========================================================================================================

    res = 0;

dev_iio_create_err:
    if (res != 0) {
        if ((*out_iio)->fd > 0) {
            close((*out_iio)->fd);
        }
        
        if ((*out_iio)->path != NULL) {
            free((*out_iio)->path);
            (*out_iio)->path = NULL;
        }
        
        if (*out_iio == NULL) {
            free(*out_iio);
            *out_iio = NULL;
        }
    }

    return res;
}

int dev_iio_change_anglvel_sampling_freq(const dev_iio_t *const iio, const char *const freq_str_hz) {
    int res = -EINVAL;
    if (!dev_iio_has_anglvel(iio)) {
        res = -ENOENT;
        goto dev_iio_change_anglvel_sampling_freq_err;
    }

    res = write_file(iio->path, "/in_anglvel_sampling_frequency", freq_str_hz, strlen(freq_str_hz));

dev_iio_change_anglvel_sampling_freq_err:
    return res;
}

int dev_iio_change_accel_sampling_freq(const dev_iio_t *const iio, const char *const freq_str_hz) {
    int res = -EINVAL;
    if (!dev_iio_has_anglvel(iio)) {
        res = -ENOENT;
        goto dev_iio_change_accel_sampling_freq_err;
    }

    res = write_file(iio->path, "/in_accel_sampling_frequency", freq_str_hz, strlen(freq_str_hz));

dev_iio_change_accel_sampling_freq_err:
    return res;
}

void dev_iio_close(dev_iio_t *const iio) {
    if (iio == NULL) {
        return;
    }

    if (iio->fd > 0) {
        close(iio->fd);
    }

    free(iio->name);
    free(iio->path);
    free(iio->dev_path);
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

// Load the kernel module
int result = syscall(__NR_finit_module, -1, "iio-trig-hrtimer", 0);
if (result == 0) {
    printf("Kernel module '%s' loaded successfully.\n", "iio-trig-hrtimer");
} else {
    perror("Error loading kernel module");
}


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
echo 1 > buffer0/in_accel_y_en
echo 1 > buffer0/in_accel_z_en
echo 1 > buffer0/enable
echo 1 > buffer/enable
cd ..
cd trigger0
echo 1 > trigger_now

# hrtimer
if [! -d "/home/config"]; then
    mkdir /home/config
fi

mount -t configfs none /home/config
mkdir /home/config/iio/triggers/hrtimer/rogue

cd /sys/bus/iio/devices/iio\:device0
echo 1 > scan_elements/in_accel_x_en
echo 1 > scan_elements/in_accel_y_en
echo 1 > scan_elements/in_accel_z_en
echo 1 > scan_elements/in_anglvel_x_en
echo 1 > scan_elements/in_anglvel_y_en
echo 1 > scan_elements/in_anglvel_z_en
echo 1 > scan_elements/in_timestamp_en
echo "rogue" > trigger/current_trigger
echo 1 > buffer0/enable
*/
//static const char *const iio_hrtrigger_name = "iio-trig-hrtimer";

static const char *const iio_path = "/sys/bus/iio/devices/";

int dev_iio_open(
    const iio_filters_t *const in_filters,
    dev_iio_t **const out_dev
) {
    int res = -ENOENT;

    char dev_path[MAX_PATH_LEN] = "\n";
    char path[MAX_PATH_LEN] = "\0";
    
    DIR *d;
    struct dirent *dir;
    d = opendir(iio_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') {
                continue;
            } else if (strstr(dir->d_name, "iio_sysfs_trigger") != NULL) {
                continue;
            } else if (strstr(dir->d_name, "trigger") != NULL) {
                continue;
            }

            snprintf(path, MAX_PATH_LEN - 1, "%s%s", iio_path, dir->d_name);
            snprintf(dev_path, MAX_PATH_LEN - 1, "/dev/%s", dir->d_name);

            // try to open the device, if it cannot be opened to go the next
            const int iio_creation_res = dev_iio_create(dev_path, path, out_dev);
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

    return res;
}

int dev_iio_has_anglvel(const dev_iio_t* iio) {
    return (iio->flags & DEV_IIO_HAS_ANGLVEL) != 0;
}

int dev_iio_has_accel(const dev_iio_t* iio) {
    return (iio->flags & DEV_IIO_HAS_ACCEL) != 0;
}

int dev_iio_get_buffer_fd(const dev_iio_t *const iio) {
    return iio->fd;
}
