#include "virt_mouse_kbd.h"

static const char* uinput_path = "/dev/uinput";

static int create_mouse_uinput() {
    int fd = open(uinput_path, O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
        fd = -1;
        close(fd);
        goto create_mouse_uinput_err;
    }
	
	struct uinput_setup dev = {0};
	strncpy(dev.name, OUTPUT_DEV_NAME, UINPUT_MAX_NAME_SIZE-1);

#if defined(OUTPUT_DEV_BUS_TYPE)
	dev.id.bustype = OUTPUT_DEV_BUS_TYPE;
#else
	dev.id.bustype = BUS_VIRTUAL;
#endif
	dev.id.vendor = OUTPUT_DEV_VENDOR_ID;
	dev.id.product = OUTPUT_DEV_PRODUCT_ID;

#if defined(OUTPUT_DEV_VERSION)
	dev.id.version = OUTPUT_DEV_VERSION;
#endif

	//if (ioctl(fd, /*UI_SET_PHYS_STR*/ 18, PHYS_STR) != 0) {
	//	fprintf(stderr, "Controller and gyroscope will NOT be recognized as a single device!\n");
	//}

	if (ioctl(fd, UI_SET_PHYS, PHYS_STR) != 0) {
		fprintf(stderr, "Error setting the phys of the virtual controller.\n");
	}

#if !defined(UI_SET_PHYS_STR)
	#warning Will not change the PHYS
#endif

#if !defined(UI_SET_UNIQ_STR)
	#warning Will not change the UNIQ
#endif

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_MSC);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
#if defined(INCLUDE_TIMESTAMP)
    ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
#endif

    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    //ioctl(fd, UI_SET_KEYBIT, BTN_SIDE);
    //ioctl(fd, UI_SET_KEYBIT, BTN_EXTRA);

    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);

    if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
        fd = -1;
        close(fd);
        goto create_mouse_uinput_err;
    }

    if(ioctl(fd, UI_DEV_CREATE) < 0) {
        fd = -1;
        close(fd);
        goto create_mouse_uinput_err;
    }
	
create_mouse_uinput_err:
    return fd;
}

static int create_kb_uinput() {

    return -EINVAL;
}


void *virt_mouse_kbd_thread_func(void *ptr) {
    devices_status_t *const stats = (devices_status_t*)ptr;

    const int mouse_fd = create_mouse_uinput();
    if (mouse_fd < 0) {
        fprintf(stderr, "Unable to create the mouse virtual evdev: %d\n", mouse_fd);
        return NULL;
    }

    keyboard_status_t prev_kbd_stats;
    kbd_status_init(&prev_kbd_stats);

    for (;;) {
        usleep(1250);
        
        const int lock_res = pthread_mutex_lock(&stats->mutex);
        if (lock_res != 0) {
            printf("Unable to lock gamepad mutex: %d\n", lock_res);

            continue;
        }

        // main virtual device logic
        {
            //event(fd, &stats->gamepad);

            if (stats->kbd.connected) {
                // TODO: do whatever it takes...
            } else {
                pthread_mutex_unlock(&stats->mutex);
                
                printf("kbd&mouse has been terminated: closing the device.\n");
                break;
            }
        }

        pthread_mutex_unlock(&stats->mutex);
    }

    return NULL;
}