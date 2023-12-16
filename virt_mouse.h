#pragma once

#include "message.h"
#include "devices_status.h"

#define VIRT_MOUSE_DEV_NAME "ROGueENEMY - mouse"

#define VIRT_MOUSE_DEV_VENDOR_ID 0x108c
#define VIRT_MOUSE_DEV_PRODUCT_ID 0x0323
#define VIRT_MOUSE_DEV_VERSION 0x0111

typedef struct virt_mouse {
    int fd;

    uint8_t prev_btn_left;
    uint8_t prev_btn_right;
    uint8_t prev_btn_middle;

    uint64_t status_recv;

} virt_mouse_t;

int virt_mouse_init(virt_mouse_t *const mouse);

int virt_mouse_get_fd(virt_mouse_t *const mouse);

int virt_mouse_send(virt_mouse_t *const mouse, mouse_status_t *const status, struct timeval *const now);

void virt_mouse_close(virt_mouse_t *const mouse);