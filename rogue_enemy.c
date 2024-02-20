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