#include "virt_ds4.h"

#include <bits/types/time_t.h>
#include <linux/uhid.h>
#include <fcntl.h>
#include <poll.h>

#define DS4_GYRO_RES_PER_DEG_S	1024
#define DS4_ACC_RES_PER_G       8192

static const uint16_t gyro_pitch_bias  = 0xfff9;
static const uint16_t gyro_yaw_bias    = 0x0009;
static const uint16_t gyro_roll_bias   = 0xfff9;
static const uint16_t gyro_pitch_plus  = 0x22fe;
static const uint16_t gyro_pitch_minus = 0xdcf4;
static const uint16_t gyro_yaw_plus    = 0x22bb;
static const uint16_t gyro_yaw_minus   = 0xdd59;
static const uint16_t gyro_roll_plus   = 0x2289;
static const uint16_t gyro_roll_minus  = 0xdd68;
static const uint16_t gyro_speed_plus  = 0x021c /* 540 */; // speed_2x = (gyro_speed_plus + gyro_speed_minus) = 1080;
static const uint16_t gyro_speed_minus = 0x021c /* 540 */; // speed_2x = (gyro_speed_plus + gyro_speed_minus) = 1080;
static const uint16_t acc_x_plus       = 0x20d3;
static const uint16_t acc_x_minus      = 0xdf07;
static const uint16_t acc_y_plus       = 0x20bf;
static const uint16_t acc_y_minus      = 0xe0aa;
static const uint16_t acc_z_plus       = 0x1ebc;
static const uint16_t acc_z_minus      = 0xe086;

static const char* path = "/dev/uhid";

static unsigned char rdesc[] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0xde, 0x28, 0x05, 0x12, 0x00, 0x02, 0x01, 0x02,
0x03, 0x01, 0x09, 0x02, 0x96, 0x00, 0x05, 0x01, 0x00, 0x80, 0xfa, 0x09, 0x04, 0x00, 0x00, 0x01,
0x03, 0x01, 0x01, 0x00, 0x09, 0x21, 0x11, 0x01, 0x21, 0x01, 0x22, 0x27, 0x00, 0x07, 0x05, 0x82,
0x03, 0x08, 0x00, 0x01, 0x09, 0x04, 0x01, 0x00, 0x01, 0x03, 0x00, 0x02, 0x00, 0x09, 0x21, 0x11,
0x01, 0x00, 0x01, 0x22, 0x41, 0x00, 0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x01, 0x09, 0x04, 0x02,
0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x19, 0x00, 0x07,
0x05, 0x83, 0x03, 0x40, 0x00, 0x01, 0x08, 0x0b, 0x03, 0x02, 0x02, 0x02, 0x01, 0x00, 0x09, 0x04,
0x03, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00, 0x05, 0x24, 0x00, 0x10, 0x01, 0x05, 0x24, 0x01, 0x01,
0x02, 0x04, 0x24, 0x02, 0x02, 0x05, 0x24, 0x06, 0x03, 0x04, 0x07, 0x05, 0x84, 0x03, 0x40, 0x00,
0xff, 0x09, 0x04, 0x04, 0x00, 0x02, 0x0a, 0x00, 0x00, 0x00, 0x07, 0x05, 0x85, 0x02, 0x40, 0x00,
0x00, 0x07, 0x05, 0x05, 0x02, 0x40, 0x00, 0x00,
};

static int uhid_write(int fd, const struct uhid_event *ev)
{
	ssize_t ret;

	ret = write(fd, ev, sizeof(*ev));
	if (ret < 0) {
		fprintf(stderr, "Cannot write to uhid: %d\n", (int)ret);
		return -errno;
	} else if (ret != sizeof(*ev)) {
		fprintf(stderr, "Wrong size written to uhid: %zd != %zu\n",
			ret, sizeof(ev));
		return -EFAULT;
	} else {
		return 0;
	}
}

static int create(int fd)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_CREATE;
	strcpy((char*)ev.u.create.name, "Sony Corp. DualShock 4 [CUH-ZCT2x]");
	ev.u.create.rd_data = rdesc;
	ev.u.create.rd_size = sizeof(rdesc);
	ev.u.create.bus = BUS_USB;
	ev.u.create.vendor = 0x28de;
	ev.u.create.product = 0x1205;
	ev.u.create.version = 0;
	ev.u.create.country = 0;

	return uhid_write(fd, &ev);
}

static void destroy(int fd)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_DESTROY;

	uhid_write(fd, &ev);
}

/* This parses raw output reports sent by the kernel to the device. A normal
 * uhid program shouldn't do this but instead just forward the raw report.
 * However, for ducomentational purposes, we try to detect LED events here and
 * print debug messages for it. */
static void handle_output(struct uhid_event *ev)
{
	/* LED messages are adverised via OUTPUT reports; ignore the rest */
	if (ev->u.output.rtype != UHID_OUTPUT_REPORT) {
        

        return;
    }
		
	/* LED reports have length 2 bytes */
	if (ev->u.output.size != 2)
		return;
	/* first byte is report-id which is 0x02 for LEDs in our rdesc */
	if (ev->u.output.data[0] != 0x2)
		return;

	/* print flags payload */
	fprintf(stderr, "LED output report received with flags %x\n",
		ev->u.output.data[1]);
}

static int event(int fd)
{
	struct uhid_event ev;
	ssize_t ret;

	memset(&ev, 0, sizeof(ev));
	ret = read(fd, &ev, sizeof(ev));
	if (ret == 0) {
		fprintf(stderr, "Read HUP on uhid-cdev\n");
		return -EFAULT;
	} else if (ret == -1) {
        return 0;
    } else if (ret < 0) {
		fprintf(stderr, "Cannot read uhid-cdev: %d\n", (int)ret);
		return -errno;
	} else if (ret != sizeof(ev)) {
		fprintf(stderr, "Invalid size read from uhid-dev: %zd != %zu\n",
			ret, sizeof(ev));
		return -EFAULT;
	}

	switch (ev.type) {
	case UHID_START:
		fprintf(stderr, "UHID_START from uhid-dev\n");
		break;
	case UHID_STOP:
		fprintf(stderr, "UHID_STOP from uhid-dev\n");
		break;
	case UHID_OPEN:
		fprintf(stderr, "UHID_OPEN from uhid-dev\n");
		break;
	case UHID_CLOSE:
		fprintf(stderr, "UHID_CLOSE from uhid-dev\n");
		break;
	case UHID_OUTPUT:
		fprintf(stderr, "UHID_OUTPUT from uhid-dev\n");
		handle_output(&ev);
		break;
	case UHID_OUTPUT_EV:
		fprintf(stderr, "UHID_OUTPUT_EV from uhid-dev\n");
		break;
    case UHID_GET_REPORT:
        fprintf(stderr, "UHID_GET_REPORT from uhid-dev, report=%d\n", ev.u.get_report.rnum);
        if (ev.u.get_report.rnum == 18) {
            const struct uhid_event mac_addr_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = 16,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            0x12, 0xf2, 0xa5, 0x71, 0x68, 0xaf, 0xdc, 0x08,
                            0x25, 0x00, 0x4c, 0x46, 0x49, 0x0e, 0x41, 0x00
                        }
                    }
                }
            };

            uhid_write(fd, &mac_addr_response);
        } else if (ev.u.get_report.rnum == 0xa3) {
            const struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = 49,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            0xa3, 0x53, 0x65, 0x70, 0x20, 0x32, 0x31, 0x20,
                            0x32, 0x30, 0x31, 0x38, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x30, 0x34, 0x3a, 0x35, 0x30, 0x3a, 0x35,
                            0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x01, 0x0c, 0xb4, 0x01, 0x00, 0x00,
                            0x00, 0x0a, 0xa0, 0x10, 0x20, 0x00, 0xa0, 0x02,
                            0x00
                        }
                    }
                }
            };

            uhid_write(fd, &firmware_info_response);
        } else if (ev.u.get_report.rnum == 0x02) { // dualshock4_get_calibration_data
            struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = 37,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x06, 0x00,

                        }
                    }
                }
            };

            // bias in kernel is 0 (embedded constant)
            // speed_2x = speed_2x*DS4_GYRO_RES_PER_DEG_S; calculated by the kernel will be 1080.
            // As a consequence sens_numer (for every axis) is 1080*1024.
            // that number will be 1105920
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[1], (const void*)&gyro_pitch_bias, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[3], (const void*)&gyro_yaw_bias, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[5], (const void*)&gyro_roll_bias, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[7], (const void*)&gyro_pitch_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[9], (const void*)&gyro_pitch_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[11], (const void*)&gyro_yaw_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[13], (const void*)&gyro_yaw_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[15], (const void*)&gyro_roll_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[17], (const void*)&gyro_roll_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[19], (const void*)&gyro_speed_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[21], (const void*)&gyro_speed_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[23], (const void*)&acc_x_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[25], (const void*)&acc_x_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[27], (const void*)&acc_y_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[29], (const void*)&acc_y_minus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[31], (const void*)&acc_z_plus, sizeof(int16_t));
            memcpy((void*)&firmware_info_response.u.get_report_reply.data[33], (const void*)&acc_z_minus, sizeof(int16_t));

            uhid_write(fd, &firmware_info_response);
        }

		break;
	default:
		fprintf(stderr, "Invalid event from uhid-dev: %u\n", ev.type);
	}

	return 0;
}

int main() {
    fprintf(stderr, "Open uhid-cdev %s\n", path);
	int fd = open(path, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Cannot open uhid-cdev %s: %d\n", path, fd);
		return NULL;
	}

	fprintf(stderr, "Create uhid device\n");
	int ret = create(fd);
	if (ret) {
		close(fd);
		return NULL;
	}

    uint8_t counter = 0;

    for (;;) {
//        if ((logic->flags & LOGIC_FLAGS_VIRT_DS4_ENABLE) != 0) {
            event(fd);

            usleep(128);

//            const int res = send_data(fd, logic, counter);
//            if (res >= 0) {
//                ++counter;
//            } else {
//                fprintf(stderr, "Error sending HID report: %d\n", res);
//            }
//        } else {
//            printf("PS4 output not enabled\n");
//        }
        
    }

    destroy(fd);

    return NULL;
}