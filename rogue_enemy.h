#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>


#include <linux/uinput.h>
#include <linux/input.h>

#include <semaphore.h>

#include <pthread.h>

#include <libevdev-1.0/libevdev/libevdev.h>

#define LSB_PER_RAD_S_2000_DEG_S ((double)0.001064724)
#define LSB_PER_RAD_S_2000_DEG_S_STR "0.001064724"

#define LSB_PER_16G ((double)0.004785)
#define LSB_PER_16G_STR "0.004785"