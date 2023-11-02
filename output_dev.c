#include "output_dev.h"

int create_output_dev(const char* uinput_path, const char* name, output_dev_type_t type) {
    int fd = open(uinput_path, O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
        fd = -1;
        close(fd);
        goto create_output_dev_err;
    }
	
	struct uinput_setup dev = {0};
	if (strlen(name) < UINPUT_MAX_NAME_SIZE) {
        strcpy(dev.name, name);
    } else {
        strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE-1);
    }

	dev.id.bustype = BUS_VIRTUAL;
	dev.id.vendor = OUTPUT_DEV_VENDOR_ID;
	dev.id.product = OUTPUT_DEV_PRODUCT_ID;
	
	switch (type) {
		case output_dev_imu: {
			ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_ACCELEROMETER);
			ioctl(fd, UI_SET_EVBIT, EV_ABS);
			//ioctl(fd, UI_SET_EVBIT, EV_KEY);
			ioctl(fd, UI_SET_EVBIT, EV_MSC);
			ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);

			ioctl(fd, UI_SET_ABSBIT, ABS_X);
			ioctl(fd, UI_SET_ABSBIT, ABS_Y);
			ioctl(fd, UI_SET_ABSBIT, ABS_Z);
			ioctl(fd, UI_SET_ABSBIT, ABS_RX);
			ioctl(fd, UI_SET_ABSBIT, ABS_RY);
			ioctl(fd, UI_SET_ABSBIT, ABS_RZ);

			//ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER);
			//ioctl(fd, UI_SET_KEYBIT, BTN_THUMB);
			
			struct uinput_abs_setup devAbsX = {0};
			devAbsX.code = ABS_X;
			devAbsX.absinfo.minimum = -ACCEL_RANGE;
			devAbsX.absinfo.maximum = ACCEL_RANGE;
			devAbsX.absinfo.resolution = 255; // 255 units = 1g
			devAbsX.absinfo.fuzz = 5;
			devAbsX.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsY = {0};
			devAbsY.code = ABS_Y;
			devAbsY.absinfo.minimum = -ACCEL_RANGE;
			devAbsY.absinfo.maximum = ACCEL_RANGE;
			devAbsY.absinfo.resolution = 255; // 255 units = 1g
			devAbsY.absinfo.fuzz = 5;
			devAbsY.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsZ = {0};
			devAbsZ.code = ABS_Z;
			devAbsZ.absinfo.minimum = -ACCEL_RANGE;
			devAbsZ.absinfo.maximum = ACCEL_RANGE;
			devAbsZ.absinfo.resolution = 255; // 255 units = 1g
			devAbsZ.absinfo.fuzz = 5;
			devAbsZ.absinfo.flat = 0;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRX = {0};
			devAbsRX.code = ABS_RX;
			devAbsRX.absinfo.minimum = -GYRO_RANGE;
			devAbsRX.absinfo.maximum = GYRO_RANGE;
			devAbsRX.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRX.absinfo.fuzz = 0;
			devAbsRX.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRX) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			struct uinput_abs_setup devAbsRY = {0};
			devAbsRY.code = ABS_RY;
			devAbsRY.absinfo.minimum = -GYRO_RANGE;
			devAbsRY.absinfo.maximum = GYRO_RANGE;
			devAbsRY.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRY.absinfo.fuzz = 0;
			devAbsRY.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRY) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
			
			struct uinput_abs_setup devAbsRZ = {0};
			devAbsRZ.code = ABS_RZ;
			devAbsRZ.absinfo.minimum = -GYRO_RANGE;
			devAbsRZ.absinfo.maximum = GYRO_RANGE;
			devAbsRZ.absinfo.resolution = 1; // 1 unit = 1 degree/s
			devAbsRZ.absinfo.fuzz = 0;
			devAbsRZ.absinfo.flat = GYRO_DEADZONE;
			if(ioctl(fd, UI_ABS_SETUP, &devAbsRZ) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}
				
			if(ioctl(fd, UI_DEV_SETUP, &dev) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			if(ioctl(fd, UI_DEV_CREATE) < 0) {
				fd = -1;
				close(fd);
				goto create_output_dev_err;
			}

			break;
		}

		case output_dev_gamepad: {

			break;
		}

		default:
			// error
			close(fd);
			fd = -1;
			goto create_output_dev_err;
	}
	
create_output_dev_err:
    return fd;
}

void *output_dev_thread_func(void *ptr) {
    output_dev_t *out_dev = (output_dev_t*)ptr;

    for (;;) {
        uint32_t flags = out_dev->crtl_flags;
		if (flags & OUTPUT_DEV_CTRL_FLAG_EXIT) {
			out_dev->crtl_flags &= ~OUTPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }
    }

    return NULL;
}
