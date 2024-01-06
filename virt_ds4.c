#include "virt_ds4.h"
#include "message.h"

#include <linux/uhid.h>

#define DS4_GYRO_RES_PER_DEG_S	1024
#define DS4_ACC_RES_PER_G       8192
#define DS4_SPEC_DELTA_TIME     188.0f

/* Flags for DualShock4 output report. */
#define DS4_OUTPUT_VALID_FLAG0_MOTOR		0x01
#define DS4_OUTPUT_VALID_FLAG0_LED		    0x02
#define DS4_OUTPUT_VALID_FLAG0_LED_BLINK	0x04

#define DS4_INPUT_REPORT_USB			0x01
#define DS4_INPUT_REPORT_USB_SIZE		64
#define DS4_INPUT_REPORT_BT			0x11
#define DS4_INPUT_REPORT_BT_SIZE		78
#define DS4_OUTPUT_REPORT_USB			0x05
#define DS4_OUTPUT_REPORT_USB_SIZE		32
#define DS4_OUTPUT_REPORT_BT			0x11
#define DS4_OUTPUT_REPORT_BT_SIZE		78

#define DS4_FEATURE_REPORT_CALIBRATION		0x02
#define DS4_FEATURE_REPORT_CALIBRATION_SIZE	37
#define DS4_FEATURE_REPORT_CALIBRATION_BT	0x05
#define DS4_FEATURE_REPORT_CALIBRATION_BT_SIZE	41
#define DS4_FEATURE_REPORT_FIRMWARE_INFO	0xa3
#define DS4_FEATURE_REPORT_FIRMWARE_INFO_SIZE	49
#define DS4_FEATURE_REPORT_PAIRING_INFO		0x12
#define DS4_FEATURE_REPORT_PAIRING_INFO_SIZE	16

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

/* Seed values for DualShock4 / DualSense CRC32 for different report types. */
static uint8_t PS_INPUT_CRC32_SEED = 0xA1;
static uint8_t PS_OUTPUT_CRC32_SEED = 0xA2;
static uint8_t PS_FEATURE_CRC32_SEED = 0xA3;

static uint32_t crc32_le(uint32_t crc_initial, const uint8_t *const buf, size_t len) {
    return crc32(crc_initial ^ 0xffffffff, buf, len) ^ 0xffffffff;
}

static const uint8_t MAC_ADDR[] = { 0xf2, 0xa5, 0x71, 0x68, 0xaf, 0xdc };

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

static unsigned char rdesc_bt[] = {
0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25,
0x07, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25,
0x01, 0x75, 0x01, 0x95, 0x0e, 0x81, 0x02, 0x75, 0x06, 0x95, 0x01, 0x81, 0x01, 0x05, 0x01, 0x09,
0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x04,
0xff, 0x85, 0x02, 0x09, 0x24, 0x95, 0x24, 0xb1, 0x02, 0x85, 0xa3, 0x09, 0x25, 0x95, 0x30, 0xb1,
0x02, 0x85, 0x05, 0x09, 0x26, 0x95, 0x28, 0xb1, 0x02, 0x85, 0x06, 0x09, 0x27, 0x95, 0x34, 0xb1,
0x02, 0x85, 0x07, 0x09, 0x28, 0x95, 0x30, 0xb1, 0x02, 0x85, 0x08, 0x09, 0x29, 0x95, 0x2f, 0xb1,
0x02, 0x85, 0x09, 0x09, 0x2a, 0x95, 0x13, 0xb1, 0x02, 0x06, 0x03, 0xff, 0x85, 0x03, 0x09, 0x21,
0x95, 0x26, 0xb1, 0x02, 0x85, 0x04, 0x09, 0x22, 0x95, 0x2e, 0xb1, 0x02, 0x85, 0xf0, 0x09, 0x47,
0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2, 0x09, 0x49,
0x95, 0x0f, 0xb1, 0x02, 0x06, 0x00, 0xff, 0x85, 0x11, 0x09, 0x20, 0x15, 0x00, 0x26, 0xff, 0x00,
0x75, 0x08, 0x95, 0x4d, 0x81, 0x02, 0x09, 0x21, 0x91, 0x02, 0x85, 0x12, 0x09, 0x22, 0x95, 0x8d,
0x81, 0x02, 0x09, 0x23, 0x91, 0x02, 0x85, 0x13, 0x09, 0x24, 0x95, 0xcd, 0x81, 0x02, 0x09, 0x25,
0x91, 0x02, 0x85, 0x14, 0x09, 0x26, 0x96, 0x0d, 0x01, 0x81, 0x02, 0x09, 0x27, 0x91, 0x02, 0x85,
0x15, 0x09, 0x28, 0x96, 0x4d, 0x01, 0x81, 0x02, 0x09, 0x29, 0x91, 0x02, 0x85, 0x16, 0x09, 0x2a,
0x96, 0x8d, 0x01, 0x81, 0x02, 0x09, 0x2b, 0x91, 0x02, 0x85, 0x17, 0x09, 0x2c, 0x96, 0xcd, 0x01,
0x81, 0x02, 0x09, 0x2d, 0x91, 0x02, 0x85, 0x18, 0x09, 0x2e, 0x96, 0x0d, 0x02, 0x81, 0x02, 0x09,
0x2f, 0x91, 0x02, 0x85, 0x19, 0x09, 0x30, 0x96, 0x22, 0x02, 0x81, 0x02, 0x09, 0x31, 0x91, 0x02,
0x06, 0x80, 0xff, 0x85, 0x82, 0x09, 0x22, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x83, 0x09, 0x23, 0xb1,
0x02, 0x85, 0x84, 0x09, 0x24, 0xb1, 0x02, 0x85, 0x90, 0x09, 0x30, 0xb1, 0x02, 0x85, 0x91, 0x09,
0x31, 0xb1, 0x02, 0x85, 0x92, 0x09, 0x32, 0xb1, 0x02, 0x85, 0x93, 0x09, 0x33, 0xb1, 0x02, 0x85,
0x94, 0x09, 0x34, 0xb1, 0x02, 0x85, 0xa0, 0x09, 0x40, 0xb1, 0x02, 0x85, 0xa4, 0x09, 0x44, 0xb1,
0x02, 0x85, 0xa7, 0x09, 0x45, 0xb1, 0x02, 0x85, 0xa8, 0x09, 0x45, 0xb1, 0x02, 0x85, 0xa9, 0x09,
0x45, 0xb1, 0x02, 0x85, 0xaa, 0x09, 0x45, 0xb1, 0x02, 0x85, 0xab, 0x09, 0x45, 0xb1, 0x02, 0x85,
0xac, 0x09, 0x45, 0xb1, 0x02, 0x85, 0xad, 0x09, 0x45, 0xb1, 0x02, 0x85, 0xb3, 0x09, 0x45, 0xb1,
0x02, 0x85, 0xb4, 0x09, 0x46, 0xb1, 0x02, 0x85, 0xb5, 0x09, 0x47, 0xb1, 0x02, 0x85, 0xd0, 0x09,
0x40, 0xb1, 0x02, 0x85, 0xd4, 0x09, 0x44, 0xb1, 0x02, 0xc0, 0x00
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
			ret, sizeof(*ev));
		return -EFAULT;
	} else {
		return 0;
	}
}

static int create(int fd, bool bluetooth)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_CREATE;
	strcpy((char*)ev.u.create.name, "Sony Corp. DualShock 4 [CUH-ZCT2x]");
	ev.u.create.rd_data = bluetooth ? rdesc_bt : rdesc;
	ev.u.create.rd_size = bluetooth ? sizeof(rdesc_bt) : sizeof(rdesc);
	ev.u.create.bus = bluetooth ? BUS_BLUETOOTH : BUS_USB;
	ev.u.create.vendor = 0x054C;
	ev.u.create.product = 0x09CC;
	ev.u.create.version = 0;
	ev.u.create.country = 0;

    sprintf((char*)ev.u.create.uniq, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        MAC_ADDR[5], MAC_ADDR[4], MAC_ADDR[3], MAC_ADDR[2], MAC_ADDR[1], MAC_ADDR[0]
    );

	return uhid_write(fd, &ev);
}

static void destroy(int fd)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_DESTROY;

	uhid_write(fd, &ev);
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

int virt_dualshock_init(
    virt_dualshock_t *const out_gamepad,
    bool bluetooth,
    int64_t gyro_to_analog_activation_treshold,
    int64_t gyro_to_analog_mapping
) {
    int ret = 0;

    out_gamepad->gyro_to_analog_activation_treshold = absolute_value(gyro_to_analog_activation_treshold);
    out_gamepad->gyro_to_analog_mapping = gyro_to_analog_mapping;
    out_gamepad->dt_sum = 0;
    out_gamepad->dt_buffer_current = 0;
    memset(out_gamepad->dt_buffer, 0, sizeof(out_gamepad->dt_buffer));
    out_gamepad->debug = false;
    out_gamepad->empty_reports = 0;
    out_gamepad->last_time = 0;
    out_gamepad->bluetooth = bluetooth;

    out_gamepad->fd = open(path, O_RDWR | O_CLOEXEC /* | O_NONBLOCK */);
    if (out_gamepad->fd < 0) {
        fprintf(stderr, "Cannot open uhid-cdev %s: %d\n", path, out_gamepad->fd);
        ret = out_gamepad->fd;
        goto virt_dualshock_init_err;
    }

    ret = create(out_gamepad->fd, out_gamepad->bluetooth);
    if (ret) {
        fprintf(stderr, "Error creating uhid device: %d\n", ret);
        close(out_gamepad->fd);
        goto virt_dualshock_init_err;
    }

virt_dualshock_init_err:
    return ret;
}

int virt_dualshock_get_fd(virt_dualshock_t *const in_gamepad) {
    return in_gamepad->fd;
}

int virt_dualshock_event(virt_dualshock_t *const gamepad, gamepad_status_t *const out_device_status) {
    struct uhid_event ev;
	ssize_t ret;

	memset(&ev, 0, sizeof(ev));
	ret = read(gamepad->fd, &ev, sizeof(ev));
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
        if (gamepad->debug) {
		    printf("UHID_START from uhid-dev\n");
        }
		break;
	case UHID_STOP:
        if (gamepad->debug) {
            printf("UHID_STOP from uhid-dev\n");
        }
        break;
	case UHID_OPEN:
        if (gamepad->debug) {
            printf("UHID_OPEN from uhid-dev\n");
        }
		break;
	case UHID_CLOSE:
        if (gamepad->debug) {
            printf("UHID_CLOSE from uhid-dev\n");
        }
		break;
	case UHID_OUTPUT:
        if (gamepad->debug) {
            printf("UHID_OUTPUT from uhid-dev\n");
        }

        // Rumble and LED messages are adverised via OUTPUT reports; ignore the rest
        if (ev.u.output.rtype != UHID_OUTPUT_REPORT)
            return 0;

        if (
            (!gamepad->bluetooth) && (ev.u.output.size != DS4_OUTPUT_REPORT_USB_SIZE) &&
            (gamepad->bluetooth) && (ev.u.output.size != DS4_OUTPUT_REPORT_BT_SIZE) 
        ) {
            fprintf(
                stderr,
                "Invalid data length: got %d, expected %d\n",
                (int)ev.u.output.size,
                (gamepad->bluetooth) ? DS4_OUTPUT_REPORT_BT_SIZE : DS4_OUTPUT_REPORT_USB_SIZE
            );

            return 0;
        }

        // first byte is report-id which is 0x01
        if (
            (!gamepad->bluetooth) && (ev.u.output.data[0] != DS4_OUTPUT_REPORT_USB) &&
            (gamepad->bluetooth) && ((ev.u.output.data[0] != DS4_OUTPUT_REPORT_BT) && (ev.u.output.data[0] < 0x10))
        ) {
            fprintf(
                stderr,
                "Unrecognised report-id: got 0x%x expected 0x%x\n",
                (int)ev.u.output.data[0],
                (gamepad->bluetooth) ? DS4_OUTPUT_REPORT_BT : DS4_OUTPUT_REPORT_USB
            );
            return 0;
        }
        
        // When using bluetooth, the first byte after the reportID is uint8_t seq_tag,
	    // while the next one is uint8_t tag, following bytes are the same.
        size_t offset = 1;
        if ((gamepad->bluetooth) && (ev.u.output.data[0] > 0x10)) {
            offset = 2;
        } else if ((gamepad->bluetooth) && (ev.u.output.data[0] == 0x02)) {
            offset = 3;
        } else if ((gamepad->bluetooth) && (ev.u.output.data[0] == 0x01)) {
            offset = 1;
        }

        const uint8_t *const common_report = &ev.u.output.data[offset];

        const uint8_t valid_flag0 = common_report[0];
        const uint8_t valid_flag1 = common_report[1];
        const uint8_t reserved = common_report[2];
        const uint8_t motor_right = common_report[3];
        const uint8_t motor_left = common_report[4];
        const uint8_t lightbar_red = common_report[5];
        const uint8_t lightbar_green = common_report[6];
        const uint8_t lightbar_blue = common_report[7];
        const uint8_t lightbar_blink_on = common_report[8];
        const uint8_t lightbar_blink_off = common_report[9];

        if ((valid_flag0 & DS4_OUTPUT_VALID_FLAG0_LED)) {
            out_device_status->leds_colors[0] = lightbar_red;
            out_device_status->leds_colors[1] = lightbar_green;
            out_device_status->leds_colors[2] = lightbar_blue;
            out_device_status->leds_events_count++;
        } else if (valid_flag0 & DS4_OUTPUT_VALID_FLAG0_LED_BLINK) {
            out_device_status->leds_colors[0] = lightbar_red;
            out_device_status->leds_colors[1] = lightbar_green;
            out_device_status->leds_colors[2] = lightbar_blue;
            out_device_status->leds_events_count++;
        }

        if ((valid_flag0 & DS4_OUTPUT_VALID_FLAG0_MOTOR)) {    
            out_device_status->motors_intensity[0] = motor_left;
            out_device_status->motors_intensity[1] = motor_right;
            out_device_status->rumble_events_count++;

            if (gamepad->debug) {
                printf(
                    "Updated rumble -- motor_left: %d, motor_right: %d, valid_flag0; %d, valid_flag1: %d\n",
                    motor_left,
                    motor_right,
                    valid_flag0,
                    valid_flag1
                );
            }
        }
		break;
	case UHID_OUTPUT_EV:
        if (gamepad->debug) {
		    printf("UHID_OUTPUT_EV from uhid-dev\n");
        }
		break;
    case UHID_GET_REPORT:
        if (gamepad->debug) {
            printf("UHID_GET_REPORT from uhid-dev, report=%d\n", ev.u.get_report.rnum);
        }
        
        if (ev.u.get_report.rnum == 18) {
            struct uhid_event mac_addr_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = DS4_FEATURE_REPORT_PAIRING_INFO_SIZE,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            DS4_FEATURE_REPORT_PAIRING_INFO,
                            MAC_ADDR[0], MAC_ADDR[1], MAC_ADDR[2], MAC_ADDR[3], MAC_ADDR[4], MAC_ADDR[5],
                            0x08,
                            0x25, 0x00, 0x4c, 0x46, 0x49, 0x0e, 0x41, 0x00
                        }
                    }
                }
            };

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&mac_addr_response.u.get_report_reply.data[0], mac_addr_response.u.get_report_reply.size - 4);
                memcpy(&mac_addr_response.u.get_report_reply.data[mac_addr_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(gamepad->fd, &mac_addr_response);
        } else if (ev.u.get_report.rnum == DS4_FEATURE_REPORT_FIRMWARE_INFO) {
            struct uhid_event firmware_info_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = DS4_FEATURE_REPORT_FIRMWARE_INFO_SIZE,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            DS4_FEATURE_REPORT_FIRMWARE_INFO, 0x53, 0x65, 0x70, 0x20, 0x32, 0x31, 0x20,
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

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&firmware_info_response.u.get_report_reply.data[0], firmware_info_response.u.get_report_reply.size - 4);
                memcpy(&firmware_info_response.u.get_report_reply.data[firmware_info_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(gamepad->fd, &firmware_info_response);
        } else if (
            ((gamepad->bluetooth) && (ev.u.get_report.rnum == DS4_FEATURE_REPORT_CALIBRATION_BT)) ||
            ((!gamepad->bluetooth) && (ev.u.get_report.rnum == DS4_FEATURE_REPORT_CALIBRATION))
        ) { // dualshock4_get_calibration_data
            struct uhid_event calibration_response = {
                .type = UHID_GET_REPORT_REPLY,
                .u = {
                    .get_report_reply = {
                        .size = gamepad->bluetooth ? DS4_FEATURE_REPORT_CALIBRATION_BT_SIZE : DS4_FEATURE_REPORT_CALIBRATION_SIZE,
                        .id = ev.u.get_report.id,
                        .err = 0,
                        .data = {
                            gamepad->bluetooth ? DS4_FEATURE_REPORT_CALIBRATION_BT : DS4_FEATURE_REPORT_CALIBRATION,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
                            0x00,
                        }
                    }
                }
            };

            // bias in kernel is 0 (embedded constant)
            // speed_2x = speed_2x*DS4_GYRO_RES_PER_DEG_S; calculated by the kernel will be 1080.
            // As a consequence sens_numer (for every axis) is 1080*1024.
            // that number will be 1105920
            memcpy((void*)&calibration_response.u.get_report_reply.data[1], (const void*)&gyro_pitch_bias, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[3], (const void*)&gyro_yaw_bias, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[5], (const void*)&gyro_roll_bias, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[7], (const void*)&gyro_pitch_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[9], (const void*)&gyro_pitch_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[11], (const void*)&gyro_yaw_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[13], (const void*)&gyro_yaw_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[15], (const void*)&gyro_roll_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[17], (const void*)&gyro_roll_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[19], (const void*)&gyro_speed_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[21], (const void*)&gyro_speed_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[23], (const void*)&acc_x_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[25], (const void*)&acc_x_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[27], (const void*)&acc_y_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[29], (const void*)&acc_y_minus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[31], (const void*)&acc_z_plus, sizeof(int16_t));
            memcpy((void*)&calibration_response.u.get_report_reply.data[33], (const void*)&acc_z_minus, sizeof(int16_t));

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&calibration_response.u.get_report_reply.data[0], calibration_response.u.get_report_reply.size - 4);
                memcpy(&calibration_response.u.get_report_reply.data[calibration_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(gamepad->fd, &calibration_response);
        }

		break;
	default:
		fprintf(stderr, "Invalid event from uhid-dev: %u\n", ev.type);
	}

	return 0;
}

void virt_dualshock_close(virt_dualshock_t *const out_gamepad) {
    destroy(out_gamepad->fd);
}

/**
 * This function arranges HID packets as described on https://www.psdevwiki.com/ps4/DS4-USB
 */
void virt_dualshock_compose(virt_dualshock_t *const gamepad, gamepad_status_t *const in_device_status, uint8_t *const out_buf) {
    const int64_t time_us = in_device_status->last_gyro_motion_timestamp_ns / (int64_t)1000;

    const int64_t delta_time = time_us - gamepad->last_time;
    gamepad->last_time = time_us;

    // find the average Δt in the last 30 non-zero inputs;
    // this has to run thousands of times a second so i'm trying to do this as fast as possible
    if (delta_time == 0) {
        gamepad->empty_reports++;
    } else if (delta_time < 1000000 && delta_time > 0 ) { // ignore outliers
        gamepad->dt_sum -= gamepad->dt_buffer[gamepad->dt_buffer_current];
        gamepad->dt_sum += delta_time;
        gamepad->dt_buffer[gamepad->dt_buffer_current] = delta_time;

        if (gamepad->dt_buffer_current == 29) {
            gamepad->dt_buffer_current = 0;
        } else {
            gamepad->dt_buffer_current++;
        }
    }

    static uint64_t sim_time = 0;
    const double correction_factor = DS4_SPEC_DELTA_TIME / ((double)gamepad->dt_sum / 30.f);
    if (delta_time != 0) {
        sim_time += (int)((double)delta_time * correction_factor);
    }

    const uint16_t timestamp = sim_time + (int)((double)gamepad->empty_reports * DS4_SPEC_DELTA_TIME);

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
   
    // buf[12] battery level

    /*
     * kernel will do:
     * int calib_data = mult_frac(ds4->gyro_calib_data[i].sens_numer, raw_data, ds4->gyro_calib_data[i].sens_denom);
     * input_report_abs(ds4->sensors, ds4->gyro_calib_data[i].abs_code, calib_data);
     * 
     * as we know sens_numer is 0, hence calib_data is zero.
     */

    const int16_t g_x = in_device_status->raw_gyro[0];
    const int16_t g_y = in_device_status->raw_gyro[1];
    const int16_t g_z = in_device_status->raw_gyro[2];
    const int16_t a_x = in_device_status->raw_accel[0];
    const int16_t a_y = in_device_status->raw_accel[1];
    const int16_t a_z = in_device_status->raw_accel[2];

    const int64_t contrib_x = (int64_t)127 + ((int64_t)g_y / (int64_t)gamepad->gyro_to_analog_mapping);
    const int64_t contrib_y = (int64_t)127 + ((int64_t)g_x / (int64_t)gamepad->gyro_to_analog_mapping);

    out_buf[0] = gamepad->bluetooth ? DS4_INPUT_REPORT_BT : DS4_INPUT_REPORT_USB;  // [00] report ID (0x01)

    uint8_t *const out_shifted_buf = gamepad->bluetooth ? &out_buf[2] : &out_buf[0];

    out_shifted_buf[1] = ((uint64_t)((int64_t)in_device_status->joystick_positions[0][0] + (int64_t)32768) >> (uint64_t)8); // L stick, X axis
    out_shifted_buf[2] = ((uint64_t)((int64_t)in_device_status->joystick_positions[0][1] + (int64_t)32768) >> (uint64_t)8); // L stick, Y axis
    out_shifted_buf[3] = ((uint64_t)((int64_t)in_device_status->joystick_positions[1][0] + (int64_t)32768) >> (uint64_t)8); // R stick, X axis
    out_shifted_buf[4] = ((uint64_t)((int64_t)in_device_status->joystick_positions[1][1] + (int64_t)32768) >> (uint64_t)8); // R stick, Y axis
    out_shifted_buf[5] =
        (in_device_status->triangle ? 0x80 : 0x00) |
        (in_device_status->circle ? 0x40 : 0x00) |
        (in_device_status->cross ? 0x20 : 0x00) |
        (in_device_status->square ? 0x10 : 0x00) |
        (uint8_t)ds4_dpad_from_gamepad(in_device_status->dpad);
    out_shifted_buf[6] =
        (in_device_status->r3 ? 0x80 : 0x00) |
        (in_device_status->l3 ? 0x40 : 0x00) | 
        (in_device_status->share ? 0x20 : 0x00) |
        (in_device_status->option ? 0x10 : 0x00) | 
        /*(in_device_status->r2_trigger > 200 ? 0x08 : 0x00)*/ 0x00 |
        /*(in_device_status->l2_trigger > 200 ? 0x04 : 0x00)*/ 0x00 |
        (in_device_status->r1 ? 0x02 : 0x00) |
        (in_device_status->l1 ? 0x01 : 0x00);
    
    if (contrib_y >= gamepad->gyro_to_analog_activation_treshold) {
        if (absolute_value(contrib_x) > gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[1] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[1] - (int64_t)127) + contrib_x), 0, 255);
        }
        
        if (absolute_value(contrib_y) >= gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[2] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[2] - (int64_t)127) + contrib_y), 0, 255);
        }
    }

    if (in_device_status->join_right_analog_and_gyroscope) {
        if (absolute_value(contrib_x) >= gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[3] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[3] - (int64_t)127) + contrib_x), 0, 255);
        }
        
        if (absolute_value(contrib_y) >= gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[4] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[4] - (int64_t)127) + contrib_y), 0, 255);
        }
    }
    
    out_shifted_buf[7] = in_device_status->center ? 0x01 : 0x00;

    out_shifted_buf[8] = in_device_status->l2_trigger;
    out_shifted_buf[9] = in_device_status->r2_trigger;
    memcpy(&out_shifted_buf[10], &timestamp, sizeof(timestamp));
    out_shifted_buf[12] = 0x20; // [12] battery level | this is called sensor_temparature in the kernel driver but is never used...
    memcpy(&out_shifted_buf[13], &g_x, sizeof(int16_t));
    memcpy(&out_shifted_buf[15], &g_y, sizeof(int16_t));
    memcpy(&out_shifted_buf[17], &g_z, sizeof(int16_t));
    memcpy(&out_shifted_buf[19], &a_x, sizeof(int16_t));
    memcpy(&out_shifted_buf[21], &a_y, sizeof(int16_t));
    memcpy(&out_shifted_buf[23], &a_z, sizeof(int16_t));

    out_shifted_buf[30] = 0x1b; // no headset attached

    out_shifted_buf[62] = 0x80; // IDK... it seems constant...
    out_shifted_buf[57] = 0x80; // IDK... it seems constant...
    out_shifted_buf[53] = 0x80; // IDK... it seems constant...
    out_shifted_buf[48] = 0x80; // IDK... it seems constant...
    out_shifted_buf[35] = 0x80; // IDK... it seems constant...
    out_shifted_buf[44] = 0x80; // IDK... it seems constant...
}

int virt_dualshock_send(virt_dualshock_t *const gamepad, uint8_t *const out_buf) {
    struct uhid_event l = {
        .type = UHID_INPUT2,
        .u = {
            .input2 = {
                .size = gamepad->bluetooth ? DS4_INPUT_REPORT_BT_SIZE : DS4_INPUT_REPORT_USB_SIZE,
            }
        }
    };

    memcpy(&l.u.input2.data[0], &out_buf[0], l.u.input2.size);

    if (gamepad->bluetooth) {
        uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_INPUT_CRC32_SEED, sizeof(PS_INPUT_CRC32_SEED));
        crc = ~crc32_le(crc, (const uint8_t *)&l.u.input2.data[0], l.u.input2.size - 4);
        memcpy(&l.u.input2.data[l.u.input2.size - sizeof(crc)], &crc, sizeof(crc));
    }

    return uhid_write(gamepad->fd, &l);
}
