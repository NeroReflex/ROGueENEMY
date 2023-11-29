#include "virt_ds4.h"

#include <linux/uhid.h>

#define DS_FEATURE_REPORT_PAIRING_INFO      0x09
#define DS_FEATURE_REPORT_PAIRING_INFO_SIZE 20

#define DS_FEATURE_REPORT_FIRMWARE_INFO         0x20
#define DS_FEATURE_REPORT_FIRMWARE_INFO_SIZE    64

#define DS_FEATURE_REPORT_CALIBRATION       0x05
#define DS_FEATURE_REPORT_CALIBRATION_SIZE  41

#define DS_INPUT_REPORT_USB_SIZE 64

static const char* path = "/dev/uhid";

static const char* const MAC_ADDR_STR = "e8:47:3a:d6:e7:74";
static const uint8_t MAC_ADDR[] = { 0x74, 0xe7, 0xd6, 0x3a, 0x47, 0xe8 };

static unsigned char rdesc[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
    0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x06, 0x81, 0x02, 0x06,
    0x00, 0xFF, 0x09, 0x20, 0x95, 0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07,
    0x35, 0x00, 0x46, 0x3B, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00, 0x05,
    0x09, 0x19, 0x01, 0x29, 0x0F, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0F, 0x81, 0x02, 0x06,
    0x00, 0xFF, 0x09, 0x21, 0x95, 0x0D, 0x81, 0x02, 0x06, 0x00, 0xFF, 0x09, 0x22, 0x15, 0x00, 0x26,
    0xFF, 0x00, 0x75, 0x08, 0x95, 0x34, 0x81, 0x02, 0x85, 0x02, 0x09, 0x23, 0x95, 0x3F, 0x91, 0x02,
    0x85, 0x05, 0x09, 0x33, 0x95, 0x28, 0xB1, 0x02, 0x85, 0x08, 0x09, 0x34, 0x95, 0x2F, 0xB1, 0x02,
    0x85, 0x09, 0x09, 0x24, 0x95, 0x13, 0xB1, 0x02, 0x85, 0x0A, 0x09, 0x25, 0x95, 0x1A, 0xB1, 0x02,
    0x85, 0x20, 0x09, 0x26, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0x21, 0x09, 0x27, 0x95, 0x04, 0xB1, 0x02,
    0x85, 0x22, 0x09, 0x40, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0x80, 0x09, 0x28, 0x95, 0x3F, 0xB1, 0x02,
    0x85, 0x81, 0x09, 0x29, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0x82, 0x09, 0x2A, 0x95, 0x09, 0xB1, 0x02,
    0x85, 0x83, 0x09, 0x2B, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0x84, 0x09, 0x2C, 0x95, 0x3F, 0xB1, 0x02,
    0x85, 0x85, 0x09, 0x2D, 0x95, 0x02, 0xB1, 0x02, 0x85, 0xA0, 0x09, 0x2E, 0x95, 0x01, 0xB1, 0x02,
    0x85, 0xE0, 0x09, 0x2F, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0xF0, 0x09, 0x30, 0x95, 0x3F, 0xB1, 0x02,
    0x85, 0xF1, 0x09, 0x31, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0xF2, 0x09, 0x32, 0x95, 0x34, 0xB1, 0x02,
    0x85, 0xF4, 0x09, 0x35, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0xF5, 0x09, 0x36, 0x95, 0x03, 0xB1, 0x02,
    0x85, 0x60, 0x09, 0x41, 0x95, 0x3F, 0xB1, 0x02, 0x85, 0x61, 0x09, 0x42, 0xB1, 0x02, 0x85, 0x62,
    0x09, 0x43, 0xB1, 0x02, 0x85, 0x63, 0x09, 0x44, 0xB1, 0x02, 0x85, 0x64, 0x09, 0x45, 0xB1, 0x02,
    0x85, 0x65, 0x09, 0x46, 0xB1, 0x02, 0x85, 0x68, 0x09, 0x47, 0xB1, 0x02, 0x85, 0x70, 0x09, 0x48,
    0xB1, 0x02, 0x85, 0x71, 0x09, 0x49, 0xB1, 0x02, 0x85, 0x72, 0x09, 0x4A, 0xB1, 0x02, 0x85, 0x73,
    0x09, 0x4B, 0xB1, 0x02, 0x85, 0x74, 0x09, 0x4C, 0xB1, 0x02, 0x85, 0x75, 0x09, 0x4D, 0xB1, 0x02,
    0x85, 0x76, 0x09, 0x4E, 0xB1, 0x02, 0x85, 0x77, 0x09, 0x4F, 0xB1, 0x02, 0x85, 0x78, 0x09, 0x50,
    0xB1, 0x02, 0x85, 0x79, 0x09, 0x51, 0xB1, 0x02, 0x85, 0x7A, 0x09, 0x52, 0xB1, 0x02, 0x85, 0x7B,
    0x09, 0x53, 0xB1, 0x02, 0xC0
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
	ev.u.create.product = 0x0df2;
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

static void handle_output(struct uhid_event *ev, const logic_t *const logic)
{
	// Rumble and LED messages are adverised via OUTPUT reports; ignore the rest
	if (ev->u.output.rtype != UHID_OUTPUT_REPORT)
        return;
	
	if (ev->u.output.size != 63) {
        fprintf(stderr, "Invalid data length: got %d, expected 63\n", (int)ev->u.output.size);

        return;
    }

	// first byte is report-id which is 0x01
	if (ev->u.output.data[0] != 0x05) {
        fprintf(stderr, "Unrecognised report-id: %d\n", (int)ev->u.output.data[0]);
        return;
    }
	
    const uint8_t valid_flag0 = ev->u.output.data[1];
	const uint8_t valid_flag1 = ev->u.output.data[2];
	const uint8_t reserved = ev->u.output.data[3];
	const uint8_t motor_right = ev->u.output.data[4];
	const uint8_t motor_left = ev->u.output.data[5];
	const uint8_t lightbar_red = ev->u.output.data[6];
	const uint8_t lightbar_green = ev->u.output.data[7];
	const uint8_t lightbar_blue = ev->u.output.data[8];
	const uint8_t lightbar_blink_on = ev->u.output.data[9];
	const uint8_t lightbar_blink_off = ev->u.output.data[10];

    // TODO dualsense logic
}

static int event(int fd, logic_t *const logic)
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
#if defined(VIRT_DS5_DEBUG)
		printf("UHID_START from uhid-dev\n");
#endif
		break;
	case UHID_STOP:
#if defined(VIRT_DS5_DEBUG)
        printf("UHID_STOP from uhid-dev\n");
#endif
        break;
	case UHID_OPEN:
#if defined(VIRT_DS5_DEBUG)
        printf("UHID_OPEN from uhid-dev\n");
#endif
		break;
	case UHID_CLOSE:
#if defined(VIRT_DS5_DEBUG)
        printf(stderr, "UHID_CLOSE from uhid-dev\n");
#endif
		break;
	case UHID_OUTPUT:
#if defined(VIRT_DS5_DEBUG)
		printf("UHID_OUTPUT from uhid-dev\n");
#endif
		handle_output(&ev, logic);
		break;
	case UHID_OUTPUT_EV:
#if defined(VIRT_DS5_DEBUG)
		printf("UHID_OUTPUT_EV from uhid-dev\n");
#endif
		break;
    case UHID_GET_REPORT:
        //fprintf(stderr, "UHID_GET_REPORT from uhid-dev, report=%d\n", ev.u.get_report.rnum);
        if (ev.u.get_report.rnum == DS_FEATURE_REPORT_PAIRING_INFO) {
            const struct uhid_event mac_addr_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = 20,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            DS_FEATURE_REPORT_PAIRING_INFO,
                            MAC_ADDR[0], MAC_ADDR[1], MAC_ADDR[2], MAC_ADDR[3], MAC_ADDR[4], MAC_ADDR[5],
                            0x08,
                            0x25, 0x00, 0x1e, 0x00, 0xee, 0x74, 0xd0, 0xbc,
                            0x00, 0x00, 0x00, 0x00
                        }
                    }
                }
            };

            uhid_write(fd, &mac_addr_response);
        } else if (ev.u.get_report.rnum == DS_FEATURE_REPORT_FIRMWARE_INFO) {
            const struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = DS_FEATURE_REPORT_FIRMWARE_INFO_SIZE,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            DS_FEATURE_REPORT_FIRMWARE_INFO,
                            0x4a, 0x75, 0x6e, 0x20, 0x31, 0x39, 0x20, 0x32, 0x30, 0x32, 0x33, 0x31, 0x34, 0x3a, 0x34,
                            0x37, 0x3a, 0x33, 0x34, 0x03, 0x00, 0x44, 0x00, 0x08, 0x02, 0x00, 0x01, 0x36, 0x00, 0x00, 0x01,
                            0xc1, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x01, 0x00, 0x00,
                            0x14, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        }
                    }
                }
            };

            uhid_write(fd, &firmware_info_response);
        } else if (ev.u.get_report.rnum == DS_FEATURE_REPORT_CALIBRATION) {
            struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = DS_FEATURE_REPORT_CALIBRATION_SIZE,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            DS_FEATURE_REPORT_CALIBRATION,
                            0xfe, 0xff, 0xfc, 0xff, 0xfe, 0xff, 0x83, 0x22, 0x78, 0xdd, 0x92, 0x22, 0x5f, 0xdd, 0x95,
                            0x22, 0x6d, 0xdd, 0x1c, 0x02, 0x1c, 0x02, 0xf2, 0x1f, 0xed, 0xdf, 0xe3, 0x20, 0xda, 0xe0, 0xee,
                            0x1f, 0xdf, 0xdf, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00,
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

static uint8_t get_buttons_byte2_by_gs(const gamepad_status_t *const gs) {
    uint8_t res = 0;

    res |= gs->r3 ? 0x80 : 0x00;
    res |= gs->l3 ? 0x40 : 0x00;
    res |= gs->share ? 0x20 : 0x00;
    res |= gs->option ? 0x10 : 0x00;

    res |= gs->r2_trigger > 200 ? 0x08 : 0x00;
    res |= gs->l2_trigger > 200 ? 0x04 : 0x00;
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

/**
 * This function arranges HID packets as described on https://www.psdevwiki.com/ps4/DS4-USB
 */
static int send_data(int fd, logic_t *const logic) {
    gamepad_status_t gs;
    const int gs_copy_res = logic_copy_gamepad_status(logic, &gs);
    if (gs_copy_res != 0) {
        fprintf(stderr, "Unable to copy the gamepad status: %d\n", gs_copy_res);
        return gs_copy_res;
    }

    const int64_t time_us = gs.last_gyro_motion_time.tv_sec * 1000000 + gs.last_gyro_motion_time.tv_usec;

    static uint32_t empty_reports = 0;
    static uint64_t last_time = 0;
    const int delta_time = time_us - last_time;
    last_time = time_us;

    // find the average Δt in the last 30 non-zero inputs;
    // this has to run thousands of times a second so i'm trying to do this as fast as possible
    static uint32_t dt_buffer[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static uint32_t dt_sum = 0;
    static uint8_t dt_buffer_current = 0; 

    if (delta_time == 0) {
        empty_reports++;
    } else if (delta_time < 1000000 && delta_time > 0 ) { // ignore outliers
        dt_sum -= dt_buffer[dt_buffer_current];
        dt_sum += delta_time;
        dt_buffer[dt_buffer_current] = delta_time;

        if (dt_buffer_current == 29) {
            dt_buffer_current = 0;
        } else {
            dt_buffer_current++;
        }
    }

/*
    static uint64_t sim_time = 0;
    const double correction_factor = DS4_SPEC_DELTA_TIME / ((double)dt_sum / 30.f);
    if (delta_time != 0) {
        sim_time += (int)((double)delta_time * correction_factor);
    }

    const uint16_t timestamp = sim_time + (int)((double)empty_reports * DS4_SPEC_DELTA_TIME);
*/
    const uint16_t timestamp = 0;
    
    // [12] battery level
    uint8_t buf[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

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
                .size = DS_INPUT_REPORT_USB_SIZE,
            }
        }
    };

    memcpy(&l.u.input2.data[0], &buf[0], l.u.input2.size);

    return uhid_write(fd, &l);
}

/**
 * Thread function emulating the DualSense controller at USB level using USB UHID ( https://www.kernel.org/doc/html/latest/hid/uhid.html ) kernel APIs.
 *
 * See https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/hid/uhid.txt?id=refs/tags/v4.10-rc3
 */
void *virt_ds5_thread_func(void *ptr) {
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
        
        event(fd, logic);

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
