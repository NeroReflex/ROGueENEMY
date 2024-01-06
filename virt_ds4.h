#pragma once

#include "message.h"
#include "devices_status.h"

/**
 * Emulator of the DualShock4 controller at USB level using USB UHID ( https://www.kernel.org/doc/html/latest/hid/uhid.html ) kernel APIs.
 *
 * See https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/hid/uhid.txt?id=refs/tags/v4.10-rc3
 */
typedef struct virt_dualshock {
    int fd;

    bool debug;

    bool bluetooth;

    uint32_t dt_sum;
    uint8_t dt_buffer_current;
    uint32_t dt_buffer[30];

    uint32_t empty_reports;
    int64_t last_time;

    int64_t gyro_to_analog_activation_treshold;
    int64_t gyro_to_analog_mapping;
} virt_dualshock_t;

int virt_dualshock_init(
    virt_dualshock_t *const gamepad,
    bool bluetooth,
    int64_t gyro_to_analog_activation_treshold,
    int64_t gyro_to_analog_mapping
);

int virt_dualshock_get_fd(
    virt_dualshock_t *const gamepad
);

int virt_dualshock_event(
    virt_dualshock_t *const gamepad,
    gamepad_status_t *const out_device_status
);

void virt_dualshock_compose(
    virt_dualshock_t *const gamepad,
    gamepad_status_t *const in_device_status,
    uint8_t *const out_buf
);

int virt_dualshock_send(
    virt_dualshock_t *const gamepad,
    uint8_t *const out_buf
);

void virt_dualshock_close(
    virt_dualshock_t *const gamepad
);
