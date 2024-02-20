#include "rogue_enemy.h"

int32_t div_round_closest(int32_t x, int32_t divisor) {
    const int32_t __x = x;
    const int32_t __d = divisor;
    return ((__x) > 0) == ((__d) > 0) ? (((__x) + ((__d) / 2)) / (__d)) : (((__x) - ((__d) / 2)) / (__d));
}

int64_t div_round_closest_i64(int64_t x, int64_t divisor) {
    const int64_t __x = x;
    const int64_t __d = divisor;
    return ((__x) > 0) == ((__d) > 0) ? (((__x) + ((__d) / 2)) / (__d)) : (((__x) - ((__d) / 2)) / (__d));
}

int64_t min_max_clamp(int64_t value, int64_t min, int64_t max) {
    if (value <= min) {
        return min;
    } else if (value >= max) {
        return max;
    }

    return value;
}

int64_t absolute_value(int64_t value) {
    if (value < 0) {
        return (int64_t)-1 * value;
    }

    return value;
}

ssize_t dmi_board_name(char *const buf, size_t buf_len) {
int dmi_name_fd = open("/sys/class/dmi/id/board_name", O_RDONLY | O_NONBLOCK);
    if (dmi_name_fd < 0) {
        return -1;
    }

    memset(buf, 0, buf_len);
    const ssize_t ret = read(dmi_name_fd, buf, buf_len);
    close(dmi_name_fd);

    return ret;
}

char* inline_read_file(const char* base_path, const char *file) {
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

int inline_write_file(const char* base_path, const char *file, const void* buf, size_t buf_sz) {
    char* fdir = NULL;

    int res = 0;

    const size_t len = strlen(base_path) + strlen(file) + 1;
    fdir = malloc(len);
    if (fdir == NULL) {
        fprintf(stderr, "Cannot allocate %ld bytes for device path, device skipped.\n", len);
        goto inline_write_file_err;
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

inline_write_file_err:
    return res;
}
