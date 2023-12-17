#pragma once

#include "message.h"
#include "devices_status.h"

#define VIRT_KBD_DEV_NAME "ROGueENEMY - kbd"
#define VIRT_KBD_DEV_VENDOR_ID 0x108c
#define VIRT_KBD_DEV_PRODUCT_ID 0x0323
#define VIRT_KBD_DEV_VERSION 0x0111

typedef struct virt_kbd {
    int fd;

    uint8_t prev_q;
    uint8_t prev_w;
    uint8_t prev_e;
    uint8_t prev_r;
    uint8_t prev_t;
    uint8_t prev_y;
    uint8_t prev_u;
    uint8_t prev_i;
    uint8_t prev_o;
    uint8_t prev_p;
    uint8_t prev_a;
    uint8_t prev_s;
    uint8_t prev_d;
    uint8_t prev_f;
    uint8_t prev_g;
    uint8_t prev_h;
    uint8_t prev_j;
    uint8_t prev_k;
    uint8_t prev_l;
    uint8_t prev_z;
    uint8_t prev_x;
    uint8_t prev_c;
    uint8_t prev_v;
    uint8_t prev_b;
    uint8_t prev_n;
    uint8_t prev_m;

    uint8_t prev_num_1;
    uint8_t prev_num_2;
    uint8_t prev_num_3;
    uint8_t prev_num_4;
    uint8_t prev_num_5;
    uint8_t prev_num_6;
    uint8_t prev_num_7;
    uint8_t prev_num_8;
    uint8_t prev_num_9;
    uint8_t prev_num_0;

    uint8_t prev_lctrl;

} virt_kbd_t;

int virt_kbd_init(virt_kbd_t *const mouse);

int virt_kbd_get_fd(virt_kbd_t *const mouse);

int virt_kbd_send(virt_kbd_t *const mouse, keyboard_status_t *const status, struct timeval *const now);

void virt_kbd_close(virt_kbd_t *const mouse);