#include "imu_output.h"

#include <linux/uinput.h>
#include <linux/input.h>

void *imu_thread_func(void *ptr) {
    output_dev_t *out_dev = (output_dev_t*)ptr;

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