#include "virt_mouse.h"

int virt_mouse_init(virt_mouse_t *const mouse) {
    int ret = -EINVAL;

    mouse->status_recv = 0;

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
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);

    struct uinput_setup dev = {0};
	strncpy(dev.name, VIRT_MOUSE_DEV_NAME, UINPUT_MAX_NAME_SIZE-1);
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = VIRT_MOUSE_DEV_VENDOR_ID;
	dev.id.product = VIRT_MOUSE_DEV_PRODUCT_ID;
    dev.id.version = VIRT_MOUSE_DEV_VERSION;

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
    mouse->prev_btn_left = 0;
    mouse->prev_btn_right = 0;
    mouse->prev_btn_middle = 0;
    mouse->fd = fd;
    ret = 0;

virt_mouse_init_err:
    if (ret != 0) {
        mouse->fd = -1;
        close(fd);
    }

    return ret;
}

int virt_mouse_get_fd(virt_mouse_t *const mouse) {
    return mouse->fd;
}

int virt_mouse_send(virt_mouse_t *const mouse, mouse_status_t *const status, struct timeval *const now) {
    int res = 0;
    
    size_t events_count = 0;
    struct input_event events[12];

    struct input_event tmp_ev;
    tmp_ev.type = EV_REL;

    if (status->x != 0) {
        tmp_ev.code = REL_X;
        tmp_ev.value = status->x;
        events[events_count++] = tmp_ev;
    }

    if (status->y != 0) {
        tmp_ev.code = REL_Y;
        tmp_ev.value = status->y;
        events[events_count++] = tmp_ev;
    }

    tmp_ev.type = EV_KEY;

    if (status->btn_left != mouse->prev_btn_left) {
        mouse->prev_btn_left = status->btn_left;
        tmp_ev.code = BTN_LEFT;
        tmp_ev.value = status->btn_left;
        events[events_count++] = tmp_ev;
    }

    if (status->btn_middle != mouse->prev_btn_middle) {
        mouse->prev_btn_middle = status->btn_middle;
        tmp_ev.code = BTN_MIDDLE;
        tmp_ev.value = status->btn_middle;
        events[events_count++] = tmp_ev;
    }

    if (status->btn_right != mouse->prev_btn_right) {
        mouse->prev_btn_right = status->btn_right;
        tmp_ev.code = BTN_RIGHT;
        tmp_ev.value = status->btn_right;
        events[events_count++] = tmp_ev;
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

    if (events_count > 0) {
        struct timeval t;
        if (now != NULL) {
            t = *now;
        } else {
            gettimeofday(&t, NULL);
        }
        
        struct input_event syn_ev = {
            .code = SYN_REPORT,
            .type = EV_SYN,
            .value = 0,
            .time = t,
        };
        syn_ev.time.tv_usec += 1;

        events[events_count++] = syn_ev;

        for (size_t i = 0; i < events_count; ++i) {
            if (i != (events_count - 1)) {
                events[i].time = t;
            }

            const ssize_t sync_written = write(mouse->fd, (void*)&events[i], sizeof(struct input_event));
            if (sync_written != sizeof(syn_ev)) {
                fprintf(stderr, "Error in sync: written %ld bytes out of %ld\n", sync_written, sizeof(syn_ev));
                res = errno;
                res = res < 0 ? res : -1*res;
                res = res == 0 ? -EIO : res;
                goto virt_mouse_send_err;
            }
        }
    }

virt_mouse_send_err:
    return res;
}

void virt_mouse_close(virt_mouse_t *const mouse) {
    close(mouse->fd);
}