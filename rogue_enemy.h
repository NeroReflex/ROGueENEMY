#pragma once

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <termios.h>
#include <dirent.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/un.h>

#include <linux/hidraw.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <linux/input.h>

#include <semaphore.h>

#include <pthread.h>

#include <libevdev-1.0/libevdev/libevdev.h>

#include <libudev.h>

#include <zlib.h>

#define LSB_PER_RAD_S_2000_DEG_S ((double)0.001064724)
#define LSB_PER_RAD_S_2000_DEG_S_STR "0.001064724"

#define LSB_PER_16G ((double)0.004785)
#define LSB_PER_16G_STR "0.004785"

#define PREFERRED_SAMPLING_FREQ ((double)800.000000)
#define PREFERRED_SAMPLING_FREQ_STR "800.000000"

#define PREFERRED_SAMPLING_FREQ_HIGH_HZ ((double)1600.000000)
#define PREFERRED_SAMPLING_FREQ_HIGH_HZ_STR "1600.000000"

// courtesy of linux kernel
#ifndef __packed
#define __packed	__attribute__((packed))
#endif

// also courtesy of linux kernel
int32_t div_round_closest(int32_t x, int32_t divisor);

int64_t div_round_closest_i64(int64_t x, int64_t divisor);

int64_t min_max_clamp(int64_t value, int64_t min, int64_t max);

int64_t absolute_value(int64_t value);

ssize_t dmi_board_name(char *const buf, size_t buf_len);

char* inline_read_file(const char* base_path, const char *file);

int inline_write_file(const char* base_path, const char *file, const void* buf, size_t buf_sz);