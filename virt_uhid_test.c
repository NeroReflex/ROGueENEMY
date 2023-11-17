

/*
 * UHID Example
 * This example emulates a basic 3 buttons mouse with wheel over UHID. Run this
 * program as root and then use the following keys to control the mouse:
 *   q: Quit the application
 *   1: Toggle left button (down, up, ...)
 *   2: Toggle right button
 *   3: Toggle middle button
 *   a: Move mouse left
 *   d: Move mouse right
 *   w: Move mouse up
 *   s: Move mouse down
 *   r: Move wheel up
 *   f: Move wheel down
 *
 * Additionally to 3 button mouse, 3 keyboard LEDs are also supported (LED_NUML,
 * LED_CAPSL and LED_SCROLLL). The device doesn't generate any related keyboard
 * events, though. You need to manually write the EV_LED/LED_XY/1 activation
 * input event to the evdev device to see it being sent to this device.
 *
 * If uhid is not available as /dev/uhid, then you can pass a different path as
 * first argument.
 * If <linux/uhid.h> is not installed in /usr, then compile this with:
 *   gcc -o ./uhid_test -Wall -I./include ./samples/uhid/uhid-example.c
 * And ignore the warning about kernel headers. However, it is recommended to
 * use the installed uhid.h if available.
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <linux/uhid.h>

/*
 * HID Report Desciptor
 * We emulate a basic 3 button mouse with wheel and 3 keyboard LEDs. This is
 * the report-descriptor as the kernel will parse it:
 *
 * INPUT(1)[INPUT]
 *   Field(0)
 *     Physical(GenericDesktop.Pointer)
 *     Application(GenericDesktop.Mouse)
 *     Usage(3)
 *       Button.0001
 *       Button.0002
 *       Button.0003
 *     Logical Minimum(0)
 *     Logical Maximum(1)
 *     Report Size(1)
 *     Report Count(3)
 *     Report Offset(0)
 *     Flags( Variable Absolute )
 *   Field(1)
 *     Physical(GenericDesktop.Pointer)
 *     Application(GenericDesktop.Mouse)
 *     Usage(3)
 *       GenericDesktop.X
 *       GenericDesktop.Y
 *       GenericDesktop.Wheel
 *     Logical Minimum(-128)
 *     Logical Maximum(127)
 *     Report Size(8)
 *     Report Count(3)
 *     Report Offset(8)
 *     Flags( Variable Relative )
 * OUTPUT(2)[OUTPUT]
 *   Field(0)
 *     Application(GenericDesktop.Keyboard)
 *     Usage(3)
 *       LED.NumLock
 *       LED.CapsLock
 *       LED.ScrollLock
 *     Logical Minimum(0)
 *     Logical Maximum(1)
 *     Report Size(1)
 *     Report Count(3)
 *     Report Offset(0)
 *     Flags( Variable Absolute )
 *
 * This is the mapping that we expect:
 *   Button.0001 ---> Key.LeftBtn
 *   Button.0002 ---> Key.RightBtn
 *   Button.0003 ---> Key.MiddleBtn
 *   GenericDesktop.X ---> Relative.X
 *   GenericDesktop.Y ---> Relative.Y
 *   GenericDesktop.Wheel ---> Relative.Wheel
 *   LED.NumLock ---> LED.NumLock
 *   LED.CapsLock ---> LED.CapsLock
 *   LED.ScrollLock ---> LED.ScrollLock
 *
 * This information can be verified by reading /sys/kernel/debug/hid/<dev>/rdesc
 * This file should print the same information as showed above.
 */

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
		fprintf(stderr, "Cannot write to uhid: %m\n");
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
	} else if (ret < 0) {
		fprintf(stderr, "Cannot read uhid-cdev: %m\n");
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
                            0x12, 0xf2, 0xa5, 0x71, 0x68, 0xaf, 0xdc, 0x08, 0x25, 0x00, 0x4c, 0x46, 0x49, 0x0e, 0x41, 0x00
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
                            /*0x40, 0x5d, 0xb0, 0xc4, 0xaa, 0x9c, 0xff, 0xff, 0x43, 0x02, 0x80, 0x08, 0x05, 0x00, 0x2d, 0x00,
                            0x93, 0x7b, 0x56, 0x65, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,*/
                            0xa3, 0x53, 0x65, 0x70, 0x20, 0x32, 0x31, 0x20, 0x32, 0x30, 0x31, 0x38, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x30, 0x34, 0x3a, 0x35, 0x30, 0x3a, 0x35, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x01, 0x0c, 0xb4, 0x01, 0x00, 0x00, 0x00, 0x0a, 0xa0, 0x10, 0x20, 0x00, 0xa0, 0x02,
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
                            /*0x40, 0x5d, 0xb0, 0xc4, 0xaa, 0x9c, 0xff, 0xff, 0x43, 0x02, 0x80, 0x08, 0x05, 0x00, 0x2d, 0x00,
                            0x93, 0x7b, 0x56, 0x65, 0x00, 0x00, 0x00, 0x00, 0xe4, 0x24, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x25, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,*/
                            0x02, 0xf9, 0xff, 0x09, 0x00, 0xf9, 0xff, 0xfe, 0x22, 0xf4, 0xdc, 0xbb, 0x22, 0x59, 0xdd, 0x89,
                            0x22, 0x68, 0xdd, 0x1c, 0x02, 0x1c, 0x02, 0xd3, 0x20, 0x07, 0xdf, 0xbf, 0x20, 0xaa, 0xe0, 0xbc,
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

static bool btn1_down;
static bool btn2_down;
static bool btn3_down;
static signed char abs_hor;
static signed char abs_ver;
static signed char wheel;

static int send_event(int fd)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_INPUT;
	ev.u.input.size = 5;

	ev.u.input.data[0] = 0x1;
	if (btn1_down)
		ev.u.input.data[1] |= 0x1;
	if (btn2_down)
		ev.u.input.data[1] |= 0x2;
	if (btn3_down)
		ev.u.input.data[1] |= 0x4;

	ev.u.input.data[2] = abs_hor;
	ev.u.input.data[3] = abs_ver;
	ev.u.input.data[4] = wheel;

	return uhid_write(fd, &ev);
}

static int keyboard(int fd)
{
	char buf[128];
	ssize_t ret, i;

	ret = read(STDIN_FILENO, buf, sizeof(buf));
	if (ret == 0) {
		fprintf(stderr, "Read HUP on stdin\n");
		return -EFAULT;
	} else if (ret < 0) {
		fprintf(stderr, "Cannot read stdin: %m\n");
		return -errno;
	}

	for (i = 0; i < ret; ++i) {
		switch (buf[i]) {
		case '1':
			btn1_down = !btn1_down;
			ret = send_event(fd);
			if (ret)
				return ret;
			break;
		case '2':
			btn2_down = !btn2_down;
			ret = send_event(fd);
			if (ret)
				return ret;
			break;
		case '3':
			btn3_down = !btn3_down;
			ret = send_event(fd);
			if (ret)
				return ret;
			break;
		case 'a':
			abs_hor = -20;
			ret = send_event(fd);
			abs_hor = 0;
			if (ret)
				return ret;
			break;
		case 'd':
			abs_hor = 20;
			ret = send_event(fd);
			abs_hor = 0;
			if (ret)
				return ret;
			break;
		case 'w':
			abs_ver = -20;
			ret = send_event(fd);
			abs_ver = 0;
			if (ret)
				return ret;
			break;
		case 's':
			abs_ver = 20;
			ret = send_event(fd);
			abs_ver = 0;
			if (ret)
				return ret;
			break;
		case 'r':
			wheel = 1;
			ret = send_event(fd);
			wheel = 0;
			if (ret)
				return ret;
			break;
		case 'f':
			wheel = -1;
			ret = send_event(fd);
			wheel = 0;
			if (ret)
				return ret;
			break;
		case 'q':
			return -ECANCELED;
		default:
			fprintf(stderr, "Invalid input: %c\n", buf[i]);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	const char *path = "/dev/uhid";
	struct pollfd pfds[2];
	int ret;
	struct termios state;

	ret = tcgetattr(STDIN_FILENO, &state);
	if (ret) {
		fprintf(stderr, "Cannot get tty state\n");
	} else {
		state.c_lflag &= ~ICANON;
		state.c_cc[VMIN] = 1;
		ret = tcsetattr(STDIN_FILENO, TCSANOW, &state);
		if (ret)
			fprintf(stderr, "Cannot set tty state\n");
	}

	if (argc >= 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			fprintf(stderr, "Usage: %s [%s]\n", argv[0], path);
			return EXIT_SUCCESS;
		} else {
			path = argv[1];
		}
	}

	fprintf(stderr, "Open uhid-cdev %s\n", path);
	fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Cannot open uhid-cdev %s: %m\n", path);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Create uhid device\n");
	ret = create(fd);
	if (ret) {
		close(fd);
		return EXIT_FAILURE;
	}

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = fd;
	pfds[1].events = POLLIN;

	fprintf(stderr, "Press 'q' to quit...\n");
	while (1) {
		ret = poll(pfds, 2, -1);
		if (ret < 0) {
			fprintf(stderr, "Cannot poll for fds: %m\n");
			break;
		}
		if (pfds[0].revents & POLLHUP) {
			fprintf(stderr, "Received HUP on stdin\n");
			break;
		}
		if (pfds[1].revents & POLLHUP) {
			fprintf(stderr, "Received HUP on uhid-cdev\n");
			break;
		}

		if (pfds[0].revents & POLLIN) {
			ret = keyboard(fd);
			if (ret)
				break;
		}
		if (pfds[1].revents & POLLIN) {
			ret = event(fd);
			if (ret)
				break;
		}
	}

	fprintf(stderr, "Destroy uhid device\n");
	destroy(fd);
	return EXIT_SUCCESS;
}
