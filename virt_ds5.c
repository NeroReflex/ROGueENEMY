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
    //Sony Interactive Entertainment DualSense Edge Wireless Controller
    0x05, 0x01,                    // Usage Page (Generic Desktop)        0
    0x09, 0x05,                    // Usage (Game Pad)                    2
    0xa1, 0x01,                    // Collection (Application)            4
    0x85, 0x01,                    //  Report ID (1)                      6
    0x09, 0x30,                    //  Usage (X)                          8
    0x09, 0x31,                    //  Usage (Y)                          10
    0x09, 0x32,                    //  Usage (Z)                          12
    0x09, 0x35,                    //  Usage (Rz)                         14
    0x09, 0x33,                    //  Usage (Rx)                         16
    0x09, 0x34,                    //  Usage (Ry)                         18
    0x15, 0x00,                    //  Logical Minimum (0)                20
    0x26, 0xff, 0x00,              //  Logical Maximum (255)              22
    0x75, 0x08,                    //  Report Size (8)                    25
    0x95, 0x06,                    //  Report Count (6)                   27
    0x81, 0x02,                    //  Input (Data,Var,Abs)               29
    0x06, 0x00, 0xff,              //  Usage Page (Vendor Defined Page 1) 31
    0x09, 0x20,                    //  Usage (Vendor Usage 0x20)          34
    0x95, 0x01,                    //  Report Count (1)                   36
    0x81, 0x02,                    //  Input (Data,Var,Abs)               38
    0x05, 0x01,                    //  Usage Page (Generic Desktop)       40
    0x09, 0x39,                    //  Usage (Hat switch)                 42
    0x15, 0x00,                    //  Logical Minimum (0)                44
    0x25, 0x07,                    //  Logical Maximum (7)                46
    0x35, 0x00,                    //  Physical Minimum (0)               48
    0x46, 0x3b, 0x01,              //  Physical Maximum (315)             50
    0x65, 0x14,                    //  Unit (EnglishRotation: deg)        53
    0x75, 0x04,                    //  Report Size (4)                    55
    0x95, 0x01,                    //  Report Count (1)                   57
    0x81, 0x42,                    //  Input (Data,Var,Abs,Null)          59
    0x65, 0x00,                    //  Unit (None)                        61
    0x05, 0x09,                    //  Usage Page (Button)                63
    0x19, 0x01,                    //  Usage Minimum (1)                  65
    0x29, 0x0f,                    //  Usage Maximum (15)                 67
    0x15, 0x00,                    //  Logical Minimum (0)                69
    0x25, 0x01,                    //  Logical Maximum (1)                71
    0x75, 0x01,                    //  Report Size (1)                    73
    0x95, 0x0f,                    //  Report Count (15)                  75
    0x81, 0x02,                    //  Input (Data,Var,Abs)               77
    0x06, 0x00, 0xff,              //  Usage Page (Vendor Defined Page 1) 79
    0x09, 0x21,                    //  Usage (Vendor Usage 0x21)          82
    0x95, 0x0d,                    //  Report Count (13)                  84
    0x81, 0x02,                    //  Input (Data,Var,Abs)               86
    0x06, 0x00, 0xff,              //  Usage Page (Vendor Defined Page 1) 88
    0x09, 0x22,                    //  Usage (Vendor Usage 0x22)          91
    0x15, 0x00,                    //  Logical Minimum (0)                93
    0x26, 0xff, 0x00,              //  Logical Maximum (255)              95
    0x75, 0x08,                    //  Report Size (8)                    98
    0x95, 0x34,                    //  Report Count (52)                  100
    0x81, 0x02,                    //  Input (Data,Var,Abs)               102
    0x85, 0x02,                    //  Report ID (2)                      104
    0x09, 0x23,                    //  Usage (Vendor Usage 0x23)          106
    0x95, 0x3f,                    //  Report Count (63)                  108
    0x91, 0x02,                    //  Output (Data,Var,Abs)              110
    0x85, 0x05,                    //  Report ID (5)                      112
    0x09, 0x33,                    //  Usage (Vendor Usage 0x33)          114
    0x95, 0x28,                    //  Report Count (40)                  116
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             118
    0x85, 0x08,                    //  Report ID (8)                      120
    0x09, 0x34,                    //  Usage (Vendor Usage 0x34)          122
    0x95, 0x2f,                    //  Report Count (47)                  124
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             126
    0x85, 0x09,                    //  Report ID (9)                      128
    0x09, 0x24,                    //  Usage (Vendor Usage 0x24)          130
    0x95, 0x13,                    //  Report Count (19)                  132
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             134
    0x85, 0x0a,                    //  Report ID (10)                     136
    0x09, 0x25,                    //  Usage (Vendor Usage 0x25)          138
    0x95, 0x1a,                    //  Report Count (26)                  140
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             142
    0x85, 0x20,                    //  Report ID (32)                     144
    0x09, 0x26,                    //  Usage (Vendor Usage 0x26)          146
    0x95, 0x3f,                    //  Report Count (63)                  148
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             150
    0x85, 0x21,                    //  Report ID (33)                     152
    0x09, 0x27,                    //  Usage (Vendor Usage 0x27)          154
    0x95, 0x04,                    //  Report Count (4)                   156
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             158
    0x85, 0x22,                    //  Report ID (34)                     160
    0x09, 0x40,                    //  Usage (Vendor Usage 0x40)          162
    0x95, 0x3f,                    //  Report Count (63)                  164
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             166
    0x85, 0x80,                    //  Report ID (128)                    168
    0x09, 0x28,                    //  Usage (Vendor Usage 0x28)          170
    0x95, 0x3f,                    //  Report Count (63)                  172
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             174
    0x85, 0x81,                    //  Report ID (129)                    176
    0x09, 0x29,                    //  Usage (Vendor Usage 0x29)          178
    0x95, 0x3f,                    //  Report Count (63)                  180
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             182
    0x85, 0x82,                    //  Report ID (130)                    184
    0x09, 0x2a,                    //  Usage (Vendor Usage 0x2a)          186
    0x95, 0x09,                    //  Report Count (9)                   188
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             190
    0x85, 0x83,                    //  Report ID (131)                    192
    0x09, 0x2b,                    //  Usage (Vendor Usage 0x2b)          194
    0x95, 0x3f,                    //  Report Count (63)                  196
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             198
    0x85, 0x84,                    //  Report ID (132)                    200
    0x09, 0x2c,                    //  Usage (Vendor Usage 0x2c)          202
    0x95, 0x3f,                    //  Report Count (63)                  204
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             206
    0x85, 0x85,                    //  Report ID (133)                    208
    0x09, 0x2d,                    //  Usage (Vendor Usage 0x2d)          210
    0x95, 0x02,                    //  Report Count (2)                   212
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             214
    0x85, 0xa0,                    //  Report ID (160)                    216
    0x09, 0x2e,                    //  Usage (Vendor Usage 0x2e)          218
    0x95, 0x01,                    //  Report Count (1)                   220
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             222
    0x85, 0xe0,                    //  Report ID (224)                    224
    0x09, 0x2f,                    //  Usage (Vendor Usage 0x2f)          226
    0x95, 0x3f,                    //  Report Count (63)                  228
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             230
    0x85, 0xf0,                    //  Report ID (240)                    232
    0x09, 0x30,                    //  Usage (Vendor Usage 0x30)          234
    0x95, 0x3f,                    //  Report Count (63)                  236
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             238
    0x85, 0xf1,                    //  Report ID (241)                    240
    0x09, 0x31,                    //  Usage (Vendor Usage 0x31)          242
    0x95, 0x3f,                    //  Report Count (63)                  244
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             246
    0x85, 0xf2,                    //  Report ID (242)                    248
    0x09, 0x32,                    //  Usage (Vendor Usage 0x32)          250
    0x95, 0x34,                    //  Report Count (52)                  252
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             254
    0x85, 0xf4,                    //  Report ID (244)                    256
    0x09, 0x35,                    //  Usage (Vendor Usage 0x35)          258
    0x95, 0x3f,                    //  Report Count (63)                  260
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             262
    0x85, 0xf5,                    //  Report ID (245)                    264
    0x09, 0x36,                    //  Usage (Vendor Usage 0x36)          266
    0x95, 0x03,                    //  Report Count (3)                   268
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             270
    0x85, 0x60,                    //  Report ID (96)                     272
    0x09, 0x41,                    //  Usage (Vendor Usage 0x41)          274
    0x95, 0x3f,                    //  Report Count (63)                  276
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             278
    0x85, 0x61,                    //  Report ID (97)                     280
    0x09, 0x42,                    //  Usage (Vendor Usage 0x42)          282
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             284
    0x85, 0x62,                    //  Report ID (98)                     286
    0x09, 0x43,                    //  Usage (Vendor Usage 0x43)          288
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             290
    0x85, 0x63,                    //  Report ID (99)                     292
    0x09, 0x44,                    //  Usage (Vendor Usage 0x44)          294
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             296
    0x85, 0x64,                    //  Report ID (100)                    298
    0x09, 0x45,                    //  Usage (Vendor Usage 0x45)          300
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             302
    0x85, 0x65,                    //  Report ID (101)                    304
    0x09, 0x46,                    //  Usage (Vendor Usage 0x46)          306
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             308
    0x85, 0x68,                    //  Report ID (104)                    310
    0x09, 0x47,                    //  Usage (Vendor Usage 0x47)          312
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             314
    0x85, 0x70,                    //  Report ID (112)                    316
    0x09, 0x48,                    //  Usage (Vendor Usage 0x48)          318
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             320
    0x85, 0x71,                    //  Report ID (113)                    322
    0x09, 0x49,                    //  Usage (Vendor Usage 0x49)          324
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             326
    0x85, 0x72,                    //  Report ID (114)                    328
    0x09, 0x4a,                    //  Usage (Vendor Usage 0x4a)          330
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             332
    0x85, 0x73,                    //  Report ID (115)                    334
    0x09, 0x4b,                    //  Usage (Vendor Usage 0x4b)          336
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             338
    0x85, 0x74,                    //  Report ID (116)                    340
    0x09, 0x4c,                    //  Usage (Vendor Usage 0x4c)          342
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             344
    0x85, 0x75,                    //  Report ID (117)                    346
    0x09, 0x4d,                    //  Usage (Vendor Usage 0x4d)          348
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             350
    0x85, 0x76,                    //  Report ID (118)                    352
    0x09, 0x4e,                    //  Usage (Vendor Usage 0x4e)          354
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             356
    0x85, 0x77,                    //  Report ID (119)                    358
    0x09, 0x4f,                    //  Usage (Vendor Usage 0x4f)          360
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             362
    0x85, 0x78,                    //  Report ID (120)                    364
    0x09, 0x50,                    //  Usage (Vendor Usage 0x50)          366
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             368
    0x85, 0x79,                    //  Report ID (121)                    370
    0x09, 0x51,                    //  Usage (Vendor Usage 0x51)          372
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             374
    0x85, 0x7a,                    //  Report ID (122)                    376
    0x09, 0x52,                    //  Usage (Vendor Usage 0x52)          378
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             380
    0x85, 0x7b,                    //  Report ID (123)                    382
    0x09, 0x53,                    //  Usage (Vendor Usage 0x53)          384
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             386
    0xc0,                          // End Collection                      388
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
	strcpy((char*)ev.u.create.name, "Sony Interactive Entertainment DualSense Edge Wireless Controller");
	ev.u.create.rd_data = rdesc;
	ev.u.create.rd_size = sizeof(rdesc);
	ev.u.create.bus = BUS_USB;
	ev.u.create.vendor = 0x054C;
	ev.u.create.product = 0x0DF2;
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
    struct timeval first_read_time;
    static int first_read_time_arrived = 0;

    gamepad_status_t gs;
    const int gs_copy_res = logic_copy_gamepad_status(logic, &gs);
    if (gs_copy_res != 0) {
        fprintf(stderr, "Unable to copy the gamepad status: %d\n", gs_copy_res);
        return gs_copy_res;
    }

    if (first_read_time_arrived == 0) {
        first_read_time = gs.last_motion_time;
        first_read_time_arrived = 1;
    }

    // Calculate the time difference in microseconds
    const int64_t dime_diff_us = (((int64_t)gs.last_motion_time.tv_sec - (int64_t)first_read_time.tv_sec) * (int64_t)1000000) +
                                    ((int64_t)gs.last_motion_time.tv_usec - (int64_t)first_read_time.tv_usec);

    // Calculate the time difference in multiples of 0.33 microseconds
    const uint16_t timestamp = ((dime_diff_us * (int64_t)3) / (int64_t)16);

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
