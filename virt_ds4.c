#include "virt_ds4.h"

#include <linux/uhid.h>
#include <fcntl.h>
#include <poll.h>

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
            const struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = 37,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            0x02, 0xf9, 0xff, 0x09, 0x00, 0xf9, 0xff, 0xfe,
                            0x22, 0xf4, 0xdc, 0xbb, 0x22, 0x59, 0xdd, 0x89,
                            0x22, 0x68, 0xdd, 0x1c, 0x02, 0x1c, 0x02, 0xd3,
                            0x20, 0x07, 0xdf, 0xbf, 0x20, 0xaa, 0xe0, 0xbc,
                            0x1e, 0x86, 0xe0, 0x06, 0x00,

                        }
                    }
                }
            };

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

static int send_data(int fd, logic_t *const logic, uint8_t counter) {
    static uint16_t timestamp = 188;

    gamepad_status_t gs = {};
    const int gs_copy_res = logic_copy_gamepad_status(logic, &gs);
    if (gs_copy_res != 0) {
        fprintf(stderr, "Unable to copy the gamepad status: %d\n", gs_copy_res);
        return gs_copy_res;
    }

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

    buf[0] = 0x01;  // [00] report ID (0x01)
    buf[1] = gs.joystick_positions[0][0]; // L stick, X axis
    buf[2] = gs.joystick_positions[0][1]; // L stick, Y axis
    buf[3] = gs.joystick_positions[1][0]; // R stick, X axis
    buf[4] = gs.joystick_positions[1][1]; // R stick, Y axis
    buf[5] = (get_buttons_byte_by_gs(&gs) /*<< (uint8_t)4*/) | ((uint8_t)gs.dpad);
    //buf[06] = get_buttons_byte_by_gs(&gs) | (uint8_t)(gs.dpad);
    buf[7] = (counter % (uint8_t)64) << ((uint8_t)2);
    buf[8] = gs.l2_trigger;
    buf[9] = gs.r2_trigger;
    memcpy(&buf[10], &timestamp, sizeof(timestamp));
    buf[12] = 0x20; // [12] battery level
    memcpy(&buf[13], &gs.gyro_x, sizeof(int16_t));
    memcpy(&buf[15], &gs.gyro_y, sizeof(int16_t));
    memcpy(&buf[17], &gs.gyro_z, sizeof(int16_t));
    memcpy(&buf[19], &gs.accel_x, sizeof(int16_t));
    memcpy(&buf[21], &gs.accel_y, sizeof(int16_t));
    memcpy(&buf[23], &gs.accel_z, sizeof(int16_t));

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

    ++timestamp;

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

    uint8_t counter = 0;

    for (;;) {
        if ((logic->flags & LOGIC_FLAGS_VIRT_DS4_ENABLE) != 0) {
            event(fd);

            usleep(128);

            const int res = send_data(fd, logic, counter);
            if (res >= 0) {
                ++counter;
            } else {
                fprintf(stderr, "Error sending HID report: %d", res);
            }
        } else {
            printf("PS4 output not enabled");
        }
        
    }

    destroy(fd);

    return NULL;
}