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
    0x05, 0x01,         /*  Usage Page (Desktop),               */
    0x09, 0x05,         /*  Usage (Gamepad),                    */
    0xA1, 0x01,         /*  Collection (Application),           */
    0x85, 0x01,         /*      Report ID (1),                  */
    0x09, 0x30,         /*      Usage (X),                      */
    0x09, 0x31,         /*      Usage (Y),                      */
    0x09, 0x32,         /*      Usage (Z),                      */
    0x09, 0x35,         /*      Usage (Rz),                     */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x26, 0xFF, 0x00,   /*      Logical Maximum (255),          */
    0x75, 0x08,         /*      Report Size (8),                */
    0x95, 0x04,         /*      Report Count (4),               */
    0x81, 0x02,         /*      Input (Variable),               */
    0x09, 0x39,         /*      Usage (Hat Switch),             */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x25, 0x07,         /*      Logical Maximum (7),            */
    0x35, 0x00,         /*      Physical Minimum (0),           */
    0x46, 0x3B, 0x01,   /*      Physical Maximum (315),         */
    0x65, 0x14,         /*      Unit (Degrees),                 */
    0x75, 0x04,         /*      Report Size (4),                */
    0x95, 0x01,         /*      Report Count (1),               */
    0x81, 0x42,         /*      Input (Variable, Null State),   */
    0x65, 0x00,         /*      Unit,                           */
    0x05, 0x09,         /*      Usage Page (Button),            */
    0x19, 0x01,         /*      Usage Minimum (01h),            */
    0x29, 0x0E,         /*      Usage Maximum (0Eh),            */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x25, 0x01,         /*      Logical Maximum (1),            */
    0x75, 0x01,         /*      Report Size (1),                */
    0x95, 0x0E,         /*      Report Count (14),              */
    0x81, 0x02,         /*      Input (Variable),               */
    0x06, 0x00, 0xFF,   /*      Usage Page (FF00h),             */
    0x09, 0x20,         /*      Usage (20h),                    */
    0x75, 0x06,         /*      Report Size (6),                */
    0x95, 0x01,         /*      Report Count (1),               */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x25, 0x3F,         /*      Logical Maximum (63),           */
    0x81, 0x02,         /*      Input (Variable),               */
    0x05, 0x01,         /*      Usage Page (Desktop),           */
    0x09, 0x33,         /*      Usage (Rx),                     */
    0x09, 0x34,         /*      Usage (Ry),                     */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x26, 0xFF, 0x00,   /*      Logical Maximum (255),          */
    0x75, 0x08,         /*      Report Size (8),                */
    0x95, 0x02,         /*      Report Count (2),               */
    0x81, 0x02,         /*      Input (Variable),               */
    0x06, 0x00, 0xFF,   /*      Usage Page (FF00h),             */
    0x09, 0x21,         /*      Usage (21h),                    */
    0x95, 0x03,         /*      Report Count (3),               */
    0x81, 0x02,         /*      Input (Variable),               */
    0x05, 0x01,         /*      Usage Page (Desktop),           */
    0x19, 0x40,         /*      Usage Minimum (40h),            */
    0x29, 0x42,         /*      Usage Maximum (42h),            */
    0x16, 0x00, 0x80,   /*      Logical Minimum (-32768),       */
    0x26, 0x00, 0x7F,   /*      Logical Maximum (32767),        */
    0x75, 0x10,         /*      Report Size (16),               */
    0x95, 0x03,         /*      Report Count (3),               */
    0x81, 0x02,         /*      Input (Variable),               */
    0x19, 0x43,         /*      Usage Minimum (43h),            */
    0x29, 0x45,         /*      Usage Maximum (45h),            */
    0x16, 0x00, 0xE0,   /*      Logical Minimum (-8192),        */
    0x26, 0xFF, 0x1F,   /*      Logical Maximum (8191),         */
    0x95, 0x03,         /*      Report Count (3),               */
    0x81, 0x02,         /*      Input (Variable),               */
    0x06, 0x00, 0xFF,   /*      Usage Page (FF00h),             */
    0x09, 0x21,         /*      Usage (21h),                    */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x26, 0xFF, 0x00,   /*      Logical Maximum (255),          */
    0x75, 0x08,         /*      Report Size (8),                */
    0x95, 0x27,         /*      Report Count (39),              */
    0x81, 0x02,         /*      Input (Variable),               */
    0x85, 0x05,         /*      Report ID (5),                  */
    0x09, 0x22,         /*      Usage (22h),                    */
    0x95, 0x1F,         /*      Report Count (31),              */
    0x91, 0x02,         /*      Output (Variable),              */
    0x85, 0x04,         /*      Report ID (4),                  */
    0x09, 0x23,         /*      Usage (23h),                    */
    0x95, 0x24,         /*      Report Count (36),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x02,         /*      Report ID (2),                  */
    0x09, 0x24,         /*      Usage (24h),                    */
    0x95, 0x24,         /*      Report Count (36),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x08,         /*      Report ID (8),                  */
    0x09, 0x25,         /*      Usage (25h),                    */
    0x95, 0x03,         /*      Report Count (3),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x10,         /*      Report ID (16),                 */
    0x09, 0x26,         /*      Usage (26h),                    */
    0x95, 0x04,         /*      Report Count (4),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x11,         /*      Report ID (17),                 */
    0x09, 0x27,         /*      Usage (27h),                    */
    0x95, 0x02,         /*      Report Count (2),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x12,         /*      Report ID (18),                 */
    0x06, 0x02, 0xFF,   /*      Usage Page (FF02h),             */
    0x09, 0x21,         /*      Usage (21h),                    */
    0x95, 0x0F,         /*      Report Count (15),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x13,         /*      Report ID (19),                 */
    0x09, 0x22,         /*      Usage (22h),                    */
    0x95, 0x16,         /*      Report Count (22),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x14,         /*      Report ID (20),                 */
    0x06, 0x05, 0xFF,   /*      Usage Page (FF05h),             */
    0x09, 0x20,         /*      Usage (20h),                    */
    0x95, 0x10,         /*      Report Count (16),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x15,         /*      Report ID (21),                 */
    0x09, 0x21,         /*      Usage (21h),                    */
    0x95, 0x2C,         /*      Report Count (44),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x06, 0x80, 0xFF,   /*      Usage Page (FF80h),             */
    0x85, 0x80,         /*      Report ID (128),                */
    0x09, 0x20,         /*      Usage (20h),                    */
    0x95, 0x06,         /*      Report Count (6),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x81,         /*      Report ID (129),                */
    0x09, 0x21,         /*      Usage (21h),                    */
    0x95, 0x06,         /*      Report Count (6),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x82,         /*      Report ID (130),                */
    0x09, 0x22,         /*      Usage (22h),                    */
    0x95, 0x05,         /*      Report Count (5),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x83,         /*      Report ID (131),                */
    0x09, 0x23,         /*      Usage (23h),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x84,         /*      Report ID (132),                */
    0x09, 0x24,         /*      Usage (24h),                    */
    0x95, 0x04,         /*      Report Count (4),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x85,         /*      Report ID (133),                */
    0x09, 0x25,         /*      Usage (25h),                    */
    0x95, 0x06,         /*      Report Count (6),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x86,         /*      Report ID (134),                */
    0x09, 0x26,         /*      Usage (26h),                    */
    0x95, 0x06,         /*      Report Count (6),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x87,         /*      Report ID (135),                */
    0x09, 0x27,         /*      Usage (27h),                    */
    0x95, 0x23,         /*      Report Count (35),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x88,         /*      Report ID (136),                */
    0x09, 0x28,         /*      Usage (28h),                    */
    0x95, 0x22,         /*      Report Count (34),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x89,         /*      Report ID (137),                */
    0x09, 0x29,         /*      Usage (29h),                    */
    0x95, 0x02,         /*      Report Count (2),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x90,         /*      Report ID (144),                */
    0x09, 0x30,         /*      Usage (30h),                    */
    0x95, 0x05,         /*      Report Count (5),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x91,         /*      Report ID (145),                */
    0x09, 0x31,         /*      Usage (31h),                    */
    0x95, 0x03,         /*      Report Count (3),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x92,         /*      Report ID (146),                */
    0x09, 0x32,         /*      Usage (32h),                    */
    0x95, 0x03,         /*      Report Count (3),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0x93,         /*      Report ID (147),                */
    0x09, 0x33,         /*      Usage (33h),                    */
    0x95, 0x0C,         /*      Report Count (12),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA0,         /*      Report ID (160),                */
    0x09, 0x40,         /*      Usage (40h),                    */
    0x95, 0x06,         /*      Report Count (6),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA1,         /*      Report ID (161),                */
    0x09, 0x41,         /*      Usage (41h),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA2,         /*      Report ID (162),                */
    0x09, 0x42,         /*      Usage (42h),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA3,         /*      Report ID (163),                */
    0x09, 0x43,         /*      Usage (43h),                    */
    0x95, 0x30,         /*      Report Count (48),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA4,         /*      Report ID (164),                */
    0x09, 0x44,         /*      Usage (44h),                    */
    0x95, 0x0D,         /*      Report Count (13),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA5,         /*      Report ID (165),                */
    0x09, 0x45,         /*      Usage (45h),                    */
    0x95, 0x15,         /*      Report Count (21),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA6,         /*      Report ID (166),                */
    0x09, 0x46,         /*      Usage (46h),                    */
    0x95, 0x15,         /*      Report Count (21),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xF0,         /*      Report ID (240),                */
    0x09, 0x47,         /*      Usage (47h),                    */
    0x95, 0x3F,         /*      Report Count (63),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xF1,         /*      Report ID (241),                */
    0x09, 0x48,         /*      Usage (48h),                    */
    0x95, 0x3F,         /*      Report Count (63),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xF2,         /*      Report ID (242),                */
    0x09, 0x49,         /*      Usage (49h),                    */
    0x95, 0x0F,         /*      Report Count (15),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA7,         /*      Report ID (167),                */
    0x09, 0x4A,         /*      Usage (4Ah),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA8,         /*      Report ID (168),                */
    0x09, 0x4B,         /*      Usage (4Bh),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xA9,         /*      Report ID (169),                */
    0x09, 0x4C,         /*      Usage (4Ch),                    */
    0x95, 0x08,         /*      Report Count (8),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAA,         /*      Report ID (170),                */
    0x09, 0x4E,         /*      Usage (4Eh),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAB,         /*      Report ID (171),                */
    0x09, 0x4F,         /*      Usage (4Fh),                    */
    0x95, 0x39,         /*      Report Count (57),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAC,         /*      Report ID (172),                */
    0x09, 0x50,         /*      Usage (50h),                    */
    0x95, 0x39,         /*      Report Count (57),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAD,         /*      Report ID (173),                */
    0x09, 0x51,         /*      Usage (51h),                    */
    0x95, 0x0B,         /*      Report Count (11),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAE,         /*      Report ID (174),                */
    0x09, 0x52,         /*      Usage (52h),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xAF,         /*      Report ID (175),                */
    0x09, 0x53,         /*      Usage (53h),                    */
    0x95, 0x02,         /*      Report Count (2),               */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0x85, 0xB0,         /*      Report ID (176),                */
    0x09, 0x54,         /*      Usage (54h),                    */
    0x95, 0x3F,         /*      Report Count (63),              */
    0xB1, 0x02,         /*      Feature (Variable),             */
    0xC0                /*  End Collection                      */
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
	ev.u.create.vendor = 0x054C;
	ev.u.create.product = 0x09CC;
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

static uint8_t get_buttons_byte_by_gs(const gamepad_status_t *const gs) {
    uint8_t res = 0;

    res |= gs->triangle ? 0x80 : 0x00;
    res |= gs->circle ? 0x40 : 0x00;
    res |= gs->cross ? 0x20 : 0x00;
    res |= gs->square ? 0x10 : 0x00;

    return res;
}

static uint8_t get_buttons_byte2_by_gs(const gamepad_status_t *const gs) {
    uint8_t res = 0;

    res |= gs->r3 ? 0x80 : 0x00;
    res |= gs->l3 ? 0x40 : 0x00;
    res |= gs->share ? 0x20 : 0x00;
    res |= gs->option ? 0x10 : 0x00;

    //res |= gs->l2 ? 0x08 : 0x00;
    //res |= gs->r2 ? 0x04 : 0x00;
    res |= gs->r1 ? 0x02 : 0x00;
    res |= gs->l1 ? 0x01 : 0x00;

    return res;
}


static uint8_t get_buttons_byte3_by_gs(const gamepad_status_t *const gs) {
    uint8_t res = 0;

    res |= gs->center ? 0x01 : 0x00;

    return res;
}

typedef enum ds4_dpad_status {
    DPAD_N        = 0,
    DPAD_NE       = 1,
    DPAD_E        = 2,
    DPAD_SE       = 3,
    DPAD_S        = 4,
    DPAD_SW       = 5,
    DPAD_W        = 6,
    DPAD_NW       = 7,
    DPAD_RELEASED = 0x08,
} ds4_dpad_status_t;

static ds4_dpad_status_t ds4_dpad_from_gamepad(uint8_t dpad) {
    if (dpad == 0x01) {
        return DPAD_E;
    } else if (dpad == 0x02) {
        return DPAD_W;
    } else if (dpad == 0x10) {
        return DPAD_N;
    } else if (dpad == 0x20) {
        return DPAD_S;
    } else if (dpad == 0x11) {
        return DPAD_NE;
    } else if (dpad == 0x12) {
        return DPAD_NW;
    } else if (dpad == 0x21) {
        return DPAD_SE;
    } else if (dpad == 0x22) {
        return DPAD_SW;
    }

    return DPAD_RELEASED;
}

static int send_data(int fd, logic_t *const logic) {
    /* struct timeval first_read_time;
    static int first_read_time_arrived = 0; */

    gamepad_status_t gs;
    const int gs_copy_res = logic_copy_gamepad_status(logic, &gs);
    if (gs_copy_res != 0) {
        fprintf(stderr, "Unable to copy the gamepad status: %d\n", gs_copy_res);
        return gs_copy_res;
    }

    /* if (first_read_time_arrived == 0) {
        first_read_time = gs.last_motion_time;
        first_read_time_arrived = 1;
    }

    // Calculate the time difference in microseconds
    const int64_t dime_diff_us = (((int64_t)gs.last_motion_time.tv_sec - (int64_t)first_read_time.tv_sec) * (int64_t)1000000) +
                                    ((int64_t)gs.last_motion_time.tv_usec - (int64_t)first_read_time.tv_usec);

    // Calculate the time difference in multiples of 0.33 microseconds
    const uint16_t timestamp = ((dime_diff_us * (int64_t)3) / (int64_t)16); */

    // FIXME: this code provides a fake but within spec timestamp
    // this allows for certain Steam Input configurations to behave correctly
    // however, this is not ideal: for precise input the timestamp should be based off of the
    // IMU.
    static uint16_t timestamp = 0;
    timestamp += 188;

    /*
    Example data:
        0000   c0 e3 af 02 ac 9c ff ff 43 01 84 08 05 00 2d 00
        0010   93 7b 56 65 00 00 00 00 c2 aa 03 00 00 00 00 00
        0020   40 00 00 00 40 00 00 00 00 00 00 00 00 00 00 00
        0030   04 00 00 00 00 00 00 00 04 02 00 00 00 00 00 00
        // above is useless, just send what is below.
        0040   01 80 7f 80 7e 08 00 5c 00 00 7e d4 06 3b fe 0c
        0050   ff 8c fe 6a 05 4f 15 56 e8 00 00 00 00 00 1b 00
        0060   00 00 00 80 00 00 00 80 00 00 00 00 80 00 00 00
        0070   80 00 00 00 00 80 00 00 00 80 00 00 00 00 80 00
    */

    // see https://www.psdevwiki.com/ps4/DS4-USB
   
    // [12] battery level
    uint8_t buf[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    /*
     * kernel will do:
     * int calib_data = mult_frac(ds4->gyro_calib_data[i].sens_numer, raw_data, ds4->gyro_calib_data[i].sens_denom);
     * input_report_abs(ds4->sensors, ds4->gyro_calib_data[i].abs_code, calib_data);
     * 
     * as we know sens_numer is 0, hence calib_data is zero.
     */
    /*
    const int16_t g_x = ((gs.gyro[0]) * ((double)(180.0)/(double)(M_PI))) / (double)DS4_GYRO_RES_PER_DEG_S;
    const int16_t g_y = ((gs.gyro[1]) * ((double)(180.0)/(double)(M_PI))) / (double)DS4_GYRO_RES_PER_DEG_S;
    const int16_t g_z = ((gs.gyro[2]) * ((double)(180.0)/(double)(M_PI))) / (double)DS4_GYRO_RES_PER_DEG_S;
    const int16_t a_x = ((gs.accel[0]) / ((double)9.8)) / (double)DS4_ACC_RES_PER_G; // TODO: IDK how to test...
    const int16_t a_y = ((gs.accel[1]) / ((double)9.8)) / (double)DS4_ACC_RES_PER_G; // TODO: IDK how to test...
    const int16_t a_z = ((gs.accel[2]) / ((double)9.8)) / (double)DS4_ACC_RES_PER_G; // TODO: IDK how to test...
    */

    /*
    const int16_t g_x = (gs.gyro[0]) / LSB_PER_RAD_S_2000_DEG_S;
    const int16_t g_y = (gs.gyro[1]) / LSB_PER_RAD_S_2000_DEG_S;
    const int16_t g_z = (gs.gyro[2]) / LSB_PER_RAD_S_2000_DEG_S;
    const int16_t a_x = (gs.accel[0]) / LSB_PER_16G; // TODO: IDK how to test...
    const int16_t a_y = (gs.accel[1]) / LSB_PER_16G; // TODO: IDK how to test...
    const int16_t a_z = (gs.accel[2]) / LSB_PER_16G; // TODO: IDK how to test...
    */

    const int16_t g_x = gs.raw_gyro[0];
    const int16_t g_y = (int16_t)(-1) * gs.raw_gyro[1];  // Swap Y and Z
    const int16_t g_z = (int16_t)(-1) * gs.raw_gyro[2];  // Swap Y and Z
    const int16_t a_x = gs.raw_accel[0];
    const int16_t a_y = (int16_t)(-1) * gs.raw_accel[1];  // Swap Y and Z
    const int16_t a_z = (int16_t)(-1) * gs.raw_accel[2];  // Swap Y and Z


    buf[0] = 0x01;  // [00] report ID (0x01)
    buf[1] = ((uint64_t)((int64_t)gs.joystick_positions[0][0] + (int64_t)32768) >> (uint64_t)8); // L stick, X axis
    buf[2] = ((uint64_t)((int64_t)gs.joystick_positions[0][1] + (int64_t)32768) >> (uint64_t)8); // L stick, Y axis
    buf[3] = ((uint64_t)((int64_t)gs.joystick_positions[1][0] + (int64_t)32768) >> (uint64_t)8); // R stick, X axis
    buf[4] = ((uint64_t)((int64_t)gs.joystick_positions[1][1] + (int64_t)32768) >> (uint64_t)8); // R stick, Y axis
    buf[5] = get_buttons_byte_by_gs(&gs) | (uint8_t)ds4_dpad_from_gamepad(gs.dpad);
    buf[6] = get_buttons_byte2_by_gs(&gs);
    
    /*
    static uint8_t counter = 0;
    buf[7] = (((counter++) % (uint8_t)64) << ((uint8_t)2)) | get_buttons_byte3_by_gs(&gs);
    */

    buf[7] = get_buttons_byte3_by_gs(&gs);

    buf[8] = gs.l2_trigger;
    buf[9] = gs.r2_trigger;
    memcpy(&buf[10], &timestamp, sizeof(timestamp));
    buf[12] = 0x20; // [12] battery level | this is called sensor_temparature in the kernel driver but is never used...
    memcpy(&buf[13], &g_x, sizeof(int16_t));
    memcpy(&buf[15], &g_y, sizeof(int16_t));
    memcpy(&buf[17], &g_z, sizeof(int16_t));
    memcpy(&buf[19], &a_x, sizeof(int16_t));
    memcpy(&buf[21], &a_y, sizeof(int16_t));
    memcpy(&buf[23], &a_z, sizeof(int16_t));

    buf[30] = 0x1b; // no headset attached

    buf[62] = 0x80; // IDK... it seems constant...
    buf[57] = 0x80; // IDK... it seems constant...
    buf[53] = 0x80; // IDK... it seems constant...
    buf[48] = 0x80; // IDK... it seems constant...
    buf[35] = 0x80; // IDK... it seems constant...
    buf[44] = 0x80; // IDK... it seems constant...

    struct uhid_event l = {
        .type = UHID_INPUT2,
        .u = {
            .input2 = {
                .size = 64,
            }
        }
    };

    memcpy(&l.u.input2.data[0], &buf[0], l.u.input2.size);

    return uhid_write(fd, &l);
}

void *virt_ds4_thread_func(void *ptr) {
    logic_t *const logic = (logic_t*)ptr;

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

    for (;;) {
        usleep(1250);
        
        event(fd);

        if (logic->gamepad_output == GAMEPAD_OUTPUT_DS4) {
            const int res = send_data(fd, logic);
            if (res < 0) {
                fprintf(stderr, "Error sending HID report: %d\n", res);
            }
        }
    }

    destroy(fd);

    return NULL;
}
