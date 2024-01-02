#pragma once

#include "message.h"
#include "devices_status.h"

/**
 * Emulator of the DualSense controller at USB level using USB UHID ( https://www.kernel.org/doc/html/latest/hid/uhid.html ) kernel APIs.
 *
 * See https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/hid/uhid.txt?id=refs/tags/v4.10-rc3
 */
typedef struct virt_dualsense {
    int fd;

    bool debug;

    bool bluetooth;

    uint8_t seq_num;

    uint32_t dt_sum;
    uint8_t dt_buffer_current;
    uint32_t dt_buffer[30];

    uint32_t empty_reports;
    int64_t last_time;
} virt_dualsense_t;

int virt_dualsense_init(virt_dualsense_t *const gamepad, bool bluetooth);

int virt_dualsense_get_fd(virt_dualsense_t *const gamepad);

int virt_dualsense_event(virt_dualsense_t *const gamepad, gamepad_status_t *const out_device_status);

void virt_dualsense_compose(virt_dualsense_t *const gamepad, gamepad_status_t *const in_device_status, uint8_t *const out_buf);

int virt_dualsense_send(virt_dualsense_t *const gamepad, uint8_t *const out_buf);

void virt_dualsense_close(virt_dualsense_t *const gamepad);

