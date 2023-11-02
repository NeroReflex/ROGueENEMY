#include "imu_output.h"

#include <linux/uinput.h>
#include <linux/input.h>

void *imu_thread_func(void *ptr) {
    output_dev_t *out_dev = (output_dev_t*)ptr;

    /*
    int64_t secAtInit = 0;
	int64_t usecAtInit = 0;
    timeval now = {0};
	gettimeofday(&now, NULL);
	secAtInit = now.tv_sec;
	usecAtInit = now.tv_usec;*/

    for (;;) {
        pthread_mutex_lock(&out_dev->ctrl_mutex);

        if (out_dev->crtl_flags & OUTPUT_DEV_CTRL_FLAG_EXIT) {
            pthread_mutex_unlock(&out_dev->ctrl_mutex);
            break;
        }

        pthread_mutex_unlock(&out_dev->ctrl_mutex);
    }

    return NULL;
}