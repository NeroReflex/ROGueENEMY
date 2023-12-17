#include "virt_kbd.h"
#include "message.h"
#include <linux/input-event-codes.h>

int virt_kbd_init(virt_kbd_t *const kbd) {
    int ret = -EINVAL;

    memset(kbd, 0, sizeof(virt_kbd_t));

    int fd = open("/dev/uinput", O_RDWR);
	if(fd < 0) {
        ret = errno;
        goto virt_mouse_init_err;
    }

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_MSC);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
    ioctl(fd, UI_SET_KEYBIT, KEY_Q);
    ioctl(fd, UI_SET_KEYBIT, KEY_W);
    ioctl(fd, UI_SET_KEYBIT, KEY_E);
    ioctl(fd, UI_SET_KEYBIT, KEY_R);
    ioctl(fd, UI_SET_KEYBIT, KEY_T);
    ioctl(fd, UI_SET_KEYBIT, KEY_Y);
    ioctl(fd, UI_SET_KEYBIT, KEY_U);
    ioctl(fd, UI_SET_KEYBIT, KEY_I);
    ioctl(fd, UI_SET_KEYBIT, KEY_O);
    ioctl(fd, UI_SET_KEYBIT, KEY_P);
    ioctl(fd, UI_SET_KEYBIT, KEY_A);
    ioctl(fd, UI_SET_KEYBIT, KEY_S);
    ioctl(fd, UI_SET_KEYBIT, KEY_D);
    ioctl(fd, UI_SET_KEYBIT, KEY_F);
    ioctl(fd, UI_SET_KEYBIT, KEY_G);
    ioctl(fd, UI_SET_KEYBIT, KEY_H);
    ioctl(fd, UI_SET_KEYBIT, KEY_J);
    ioctl(fd, UI_SET_KEYBIT, KEY_K);
    ioctl(fd, UI_SET_KEYBIT, KEY_L);
    ioctl(fd, UI_SET_KEYBIT, KEY_Z);
    ioctl(fd, UI_SET_KEYBIT, KEY_X);
    ioctl(fd, UI_SET_KEYBIT, KEY_C);
    ioctl(fd, UI_SET_KEYBIT, KEY_V);
    ioctl(fd, UI_SET_KEYBIT, KEY_B);
    ioctl(fd, UI_SET_KEYBIT, KEY_N);
    ioctl(fd, UI_SET_KEYBIT, KEY_M);
    ioctl(fd, UI_SET_KEYBIT, KEY_0);
    ioctl(fd, UI_SET_KEYBIT, KEY_1);
    ioctl(fd, UI_SET_KEYBIT, KEY_2);
    ioctl(fd, UI_SET_KEYBIT, KEY_3);
    ioctl(fd, UI_SET_KEYBIT, KEY_4);
    ioctl(fd, UI_SET_KEYBIT, KEY_5);
    ioctl(fd, UI_SET_KEYBIT, KEY_6);
    ioctl(fd, UI_SET_KEYBIT, KEY_7);
    ioctl(fd, UI_SET_KEYBIT, KEY_8);
    ioctl(fd, UI_SET_KEYBIT, KEY_9);
    ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);
    //ioctl(fd, UI_SET_KEYBIT, KEY_);

    struct uinput_setup dev = {0};
	strncpy(dev.name, VIRT_KBD_DEV_NAME, UINPUT_MAX_NAME_SIZE-1);
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = VIRT_KBD_DEV_VENDOR_ID;
	dev.id.product = VIRT_KBD_DEV_PRODUCT_ID;
    dev.id.version = VIRT_KBD_DEV_VERSION;

    if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
        ret = errno > 0 ? errno : -1 * errno;
        ret = ret == 0 ? -EIO : ret;
        goto virt_mouse_init_err;
    }

    if(ioctl(fd, UI_DEV_CREATE) < 0) {
        ret = errno > 0 ? errno : -1 * errno;
        ret = ret == 0 ? -EIO : ret;
        goto virt_mouse_init_err;
    }

    // initialization ok
    kbd->fd = fd;
    ret = 0;

virt_mouse_init_err:
    if (ret != 0) {
        kbd->fd = -1;
        close(fd);
    }

    return ret;
}

int virt_kbd_get_fd(virt_kbd_t *const kbd) {
    return kbd->fd;
}

int virt_kbd_send(virt_kbd_t *const kbd, keyboard_status_t *const status, struct timeval *const now) {
    int res = 0;
    
    struct timeval syn_time;
    gettimeofday(&syn_time, NULL);

    struct input_event tmp_ev;
    gettimeofday(&tmp_ev.time, NULL);
    if (now != NULL) {
        tmp_ev.time = *now;
    }

    tmp_ev.type = EV_KEY;

    if (status->q != kbd->prev_q) {
        tmp_ev.code = KEY_Q;
        tmp_ev.value = kbd->prev_q = status->q;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->w != kbd->prev_w) {
        tmp_ev.code = KEY_W;
        tmp_ev.value = kbd->prev_w = status->w;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->e != kbd->prev_e) {
        tmp_ev.code = KEY_E;
        tmp_ev.value = kbd->prev_e = status->e;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->r != kbd->prev_r) {
        tmp_ev.code = KEY_R;
        tmp_ev.value = kbd->prev_r = status->r;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->t != kbd->prev_t) {
        tmp_ev.code = KEY_T;
        tmp_ev.value = kbd->prev_t = status->t;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->y != kbd->prev_y) {
        tmp_ev.code = KEY_Y;
        tmp_ev.value = kbd->prev_y = status->y;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->u != kbd->prev_u) {
        tmp_ev.code = KEY_U;
        tmp_ev.value = kbd->prev_u = status->u;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->i != kbd->prev_i) {
        tmp_ev.code = KEY_I;
        tmp_ev.value = kbd->prev_i = status->i;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->o != kbd->prev_o) {
        tmp_ev.code = KEY_O;
        tmp_ev.value = kbd->prev_o = status->o;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->p != kbd->prev_p) {
        tmp_ev.code = KEY_P;
        tmp_ev.value = kbd->prev_p = status->p;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->a != kbd->prev_a) {
        tmp_ev.code = KEY_A;
        tmp_ev.value = kbd->prev_a = status->a;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->s != kbd->prev_s) {
        tmp_ev.code = KEY_S;
        tmp_ev.value = kbd->prev_s = status->s;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }


    if (status->d != kbd->prev_d) {
        tmp_ev.code = KEY_D;
        tmp_ev.value = kbd->prev_d = status->d;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->f != kbd->prev_f) {
        tmp_ev.code = KEY_F;
        tmp_ev.value = kbd->prev_f = status->f;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->g != kbd->prev_g) {
        tmp_ev.code = KEY_G;
        tmp_ev.value = kbd->prev_g = status->g;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->h != kbd->prev_h) {
        tmp_ev.code = KEY_H;
        tmp_ev.value = kbd->prev_h = status->h;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->j != kbd->prev_j) {
        tmp_ev.code = KEY_J;
        tmp_ev.value = kbd->prev_j = status->j;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->k != kbd->prev_k) {
        tmp_ev.code = KEY_K;
        tmp_ev.value = kbd->prev_k = status->k;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->l != kbd->prev_l) {
        tmp_ev.code = KEY_L;
        tmp_ev.value = kbd->prev_l = status->l;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->z != kbd->prev_z) {
        tmp_ev.code = KEY_Z;
        tmp_ev.value = kbd->prev_z = status->z;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->x != kbd->prev_x) {
        tmp_ev.code = KEY_X;
        tmp_ev.value = kbd->prev_x = status->x;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->c != kbd->prev_c) {
        tmp_ev.code = KEY_C;
        tmp_ev.value = kbd->prev_c = status->c;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->v != kbd->prev_v) {
        tmp_ev.code = KEY_V;
        tmp_ev.value = kbd->prev_v = status->v;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->b != kbd->prev_b) {
        tmp_ev.code = KEY_B;
        tmp_ev.value = kbd->prev_b = status->b;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->n != kbd->prev_n) {
        tmp_ev.code = KEY_N;
        tmp_ev.value = kbd->prev_n = status->n;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->m != kbd->prev_m) {
        tmp_ev.code = KEY_M;
        tmp_ev.value = kbd->prev_m = status->m;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_0 != kbd->prev_num_0) {
        tmp_ev.code = KEY_0;
        tmp_ev.value = kbd->prev_num_0 = status->num_0;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_1 != kbd->prev_num_1) {
        tmp_ev.code = KEY_1;
        tmp_ev.value = kbd->prev_num_1 = status->num_1;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_2 != kbd->prev_num_2) {
        tmp_ev.code = KEY_2;
        tmp_ev.value = kbd->prev_num_2 = status->num_2;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_3 != kbd->prev_num_3) {
        tmp_ev.code = KEY_3;
        tmp_ev.value = kbd->prev_num_3 = status->num_3;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_4 != kbd->prev_num_4) {
        tmp_ev.code = KEY_4;
        tmp_ev.value = kbd->prev_num_4 = status->num_4;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_5 != kbd->prev_num_5) {
        tmp_ev.code = KEY_5;
        tmp_ev.value = kbd->prev_num_5 = status->num_5;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_6 != kbd->prev_num_6) {
        tmp_ev.code = KEY_6;
        tmp_ev.value = kbd->prev_num_6 = status->num_6;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_7 != kbd->prev_num_7) {
        tmp_ev.code = KEY_7;
        tmp_ev.value = kbd->prev_num_7 = status->num_7;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_8 != kbd->prev_num_8) {
        tmp_ev.code = KEY_8;
        tmp_ev.value = kbd->prev_num_8 = status->num_8;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->num_9 != kbd->prev_num_9) {
        tmp_ev.code = KEY_9;
        tmp_ev.value = kbd->prev_num_9 = status->num_9;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->lctrl != kbd->prev_lctrl) {
        tmp_ev.code = KEYBOARD_KEY_LCRTL;
        tmp_ev.value = kbd->prev_lctrl = status->lctrl;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->up != kbd->prev_up) {
        tmp_ev.code = KEY_LEFTCTRL;
        tmp_ev.value = kbd->prev_up = status->up;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->down != kbd->prev_down) {
        tmp_ev.code = KEY_DOWN;
        tmp_ev.value = kbd->prev_down = status->down;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->left != kbd->prev_left) {
        tmp_ev.code = KEY_LEFT;
        tmp_ev.value = kbd->prev_left = status->left;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    if (status->right != kbd->prev_right) {
        tmp_ev.code = KEY_RIGHT;
        tmp_ev.value = kbd->prev_right = status->right;
        if (write(kbd->fd, &tmp_ev, sizeof(tmp_ev)) != sizeof(struct input_event)) {
            res = errno < 0 ? errno : -1 * errno;
            goto virt_kbd_send_err;
        }
    }

    #if 0
	const struct input_event timestamp_ev = {
		.code = MSC_TIMESTAMP,
		.type = EV_MSC,
		.value = (now.tv_sec - secAtInit)*1000000 + (now.tv_usec - usecAtInit),
		.time = now,
	};
	const ssize_t timestamp_written = write(fd, (void*)&timestamp_ev, sizeof(timestamp_ev));
	if (timestamp_written != sizeof(timestamp_ev)) {
		fprintf(stderr, "Error in sync: written %ld bytes out of %ld\n", timestamp_written, sizeof(timestamp_ev));
	}
    #endif

    syn_time.tv_usec += 1;
	const struct input_event syn_ev = {
		.code = SYN_REPORT,
		.type = EV_SYN,
		.value = 0,
		.time = syn_time,
	};
	const ssize_t sync_written = write(kbd->fd, (void*)&syn_ev, sizeof(syn_ev));
	if (sync_written != sizeof(syn_ev)) {
		fprintf(stderr, "Error in sync: written %ld bytes out of %ld\n", sync_written, sizeof(syn_ev));
	}

virt_kbd_send_err:
    return res;
}

void virt_kbd_close(virt_kbd_t *const kbd) {
    close(kbd->fd);
}