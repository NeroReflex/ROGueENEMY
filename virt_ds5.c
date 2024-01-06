#include "virt_ds5.h"
#include "message.h"
#include "rogue_enemy.h"

#include <linux/uhid.h>

#define DS_FEATURE_REPORT_PAIRING_INFO      0x09
#define DS_FEATURE_REPORT_PAIRING_INFO_SIZE 20

#define DS_FEATURE_REPORT_FIRMWARE_INFO         0x20
#define DS_FEATURE_REPORT_FIRMWARE_INFO_SIZE    64

#define DS_FEATURE_REPORT_CALIBRATION       0x05
#define DS_FEATURE_REPORT_CALIBRATION_SIZE  41

#define DS_INPUT_REPORT_USB         0x01
#define DS_INPUT_REPORT_USB_SIZE    64
#define DS_INPUT_REPORT_BT          0x31
#define DS_INPUT_REPORT_BT_SIZE     78
#define DS_OUTPUT_REPORT_USB        0x02
#define DS_OUTPUT_REPORT_USB_SIZE   63
#define DS_OUTPUT_REPORT_BT         0x31
#define DS_OUTPUT_REPORT_BT_SIZE    78

#define DS_OUTPUT_VALID_FLAG0_HAPTICS_SELECT    0x02

#define DS_OUTPUT_VALID_FLAG2_COMPATIBLE_VIBRATION2 0x04
#define DS_OUTPUT_VALID_FLAG0_COMPATIBLE_VIBRATION  0x01

#define DS_OUTPUT_VALID_FLAG1_LIGHTBAR_CONTROL_ENABLE 0x04

#define DS5_SPEC_DELTA_TIME         4096.0f

static uint32_t crc32_le(uint32_t crc_initial, const uint8_t *const buf, size_t len) {
    return crc32(crc_initial ^ 0xffffffff, buf, len) ^ 0xffffffff;
}

/* Seed values for DualShock4 / DualSense CRC32 for different report types. */
static uint8_t PS_INPUT_CRC32_SEED = 0xA1;
static uint8_t PS_OUTPUT_CRC32_SEED = 0xA2;
static uint8_t PS_FEATURE_CRC32_SEED = 0xA3;

#define DS5_EDGE_NAME "Sony Interactive Entertainment DualSense Edge Wireless Controller"
#define DS5_EDGE_VERSION 256
#define DS5_EDGE_VENDOR 0x054C
#define DS5_EDGE_PRODUCT 0x0DF2

#define DS5_NAME "Sony Interactive Entertainment DualSense Wireless Controller"
#define DS5_VERSION 0x8111
#define DS5_VENDOR 0x054C
#define DS5_PRODUCT 0x0ce6

static const char* path = "/dev/uhid";

//static const char* const MAC_ADDR_STR = "e8:47:3a:d6:e7:74";
static const uint8_t MAC_ADDR[] = { 0x74, 0xe7, 0xd6, 0x3a, 0x47, 0xe8 };

static unsigned char rdesc_edge[] = {
    0x05,
        0x01,  // Usage Page (Generic Desktop)        0
        0x09,
        0x05,  // Usage (Game Pad)                    2
        0xA1,
        0x01,  // Collection (Application)            4
        0x85,
        0x01,  //  Report ID (1)                      6
        0x09,
        0x30,  //  Usage (X)                          8
        0x09,
        0x31,  //  Usage (Y)                          10
        0x09,
        0x32,  //  Usage (Z)                          12
        0x09,
        0x35,  //  Usage (Rz)                         14
        0x09,
        0x33,  //  Usage (Rx)                         16
        0x09,
        0x34,  //  Usage (Ry)                         18
        0x15,
        0x00,  //  Logical Minimum (0)                20
        0x26,
        0xFF,
        0x00,  //  Logical Maximum (255)              22
        0x75,
        0x08,  //  Report Size (8)                    25
        0x95,
        0x06,  //  Report Count (6)                   27
        0x81,
        0x02,  //  Input (Data,Var,Abs)               29
        0x06,
        0x00,
        0xFF,  //  Usage Page (Vendor Defined Page 1) 31
        0x09,
        0x20,  //  Usage (Vendor Usage 0x20)          34
        0x95,
        0x01,  //  Report Count (1)                   36
        0x81,
        0x02,  //  Input (Data,Var,Abs)               38
        0x05,
        0x01,  //  Usage Page (Generic Desktop)       40
        0x09,
        0x39,  //  Usage (Hat switch)                 42
        0x15,
        0x00,  //  Logical Minimum (0)                44
        0x25,
        0x07,  //  Logical Maximum (7)                46
        0x35,
        0x00,  //  Physical Minimum (0)               48
        0x46,
        0x3B,
        0x01,  //  Physical Maximum (315)             50
        0x65,
        0x14,  //  Unit (EnglishRotation: deg)        53
        0x75,
        0x04,  //  Report Size (4)                    55
        0x95,
        0x01,  //  Report Count (1)                   57
        0x81,
        0x42,  //  Input (Data,Var,Abs,Null)          59
        0x65,
        0x00,  //  Unit (None)                        61
        0x05,
        0x09,  //  Usage Page (Button)                63
        0x19,
        0x01,  //  Usage Minimum (1)                  65
        0x29,
        0x0F,  //  Usage Maximum (15)                 67
        0x15,
        0x00,  //  Logical Minimum (0)                69
        0x25,
        0x01,  //  Logical Maximum (1)                71
        0x75,
        0x01,  //  Report Size (1)                    73
        0x95,
        0x0F,  //  Report Count (15)                  75
        0x81,
        0x02,  //  Input (Data,Var,Abs)               77
        0x06,
        0x00,
        0xFF,  //  Usage Page (Vendor Defined Page 1) 79
        0x09,
        0x21,  //  Usage (Vendor Usage 0x21)          82
        0x95,
        0x0D,  //  Report Count (13)                  84
        0x81,
        0x02,  //  Input (Data,Var,Abs)               86
        0x06,
        0x00,
        0xFF,  //  Usage Page (Vendor Defined Page 1) 88
        0x09,
        0x22,  //  Usage (Vendor Usage 0x22)          91
        0x15,
        0x00,  //  Logical Minimum (0)                93
        0x26,
        0xFF,
        0x00,  //  Logical Maximum (255)              95
        0x75,
        0x08,  //  Report Size (8)                    98
        0x95,
        0x34,  //  Report Count (52)                  100
        0x81,
        0x02,  //  Input (Data,Var,Abs)               102
        0x85,
        0x02,  //  Report ID (2)                      104
        0x09,
        0x23,  //  Usage (Vendor Usage 0x23)          106
        0x95,
        0x3F,  //  Report Count (63)                  108
        0x91,
        0x02,  //  Output (Data,Var,Abs)              110
        0x85,
        0x05,  //  Report ID (5)                      112
        0x09,
        0x33,  //  Usage (Vendor Usage 0x33)          114
        0x95,
        0x28,  //  Report Count (40)                  116
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             118
        0x85,
        0x08,  //  Report ID (8)                      120
        0x09,
        0x34,  //  Usage (Vendor Usage 0x34)          122
        0x95,
        0x2F,  //  Report Count (47)                  124
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             126
        0x85,
        0x09,  //  Report ID (9)                      128
        0x09,
        0x24,  //  Usage (Vendor Usage 0x24)          130
        0x95,
        0x13,  //  Report Count (19)                  132
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             134
        0x85,
        0x0A,  //  Report ID (10)                     136
        0x09,
        0x25,  //  Usage (Vendor Usage 0x25)          138
        0x95,
        0x1A,  //  Report Count (26)                  140
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             142
        0x85,
        0x20,  //  Report ID (32)                     144
        0x09,
        0x26,  //  Usage (Vendor Usage 0x26)          146
        0x95,
        0x3F,  //  Report Count (63)                  148
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             150
        0x85,
        0x21,  //  Report ID (33)                     152
        0x09,
        0x27,  //  Usage (Vendor Usage 0x27)          154
        0x95,
        0x04,  //  Report Count (4)                   156
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             158
        0x85,
        0x22,  //  Report ID (34)                     160
        0x09,
        0x40,  //  Usage (Vendor Usage 0x40)          162
        0x95,
        0x3F,  //  Report Count (63)                  164
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             166
        0x85,
        0x80,  //  Report ID (128)                    168
        0x09,
        0x28,  //  Usage (Vendor Usage 0x28)          170
        0x95,
        0x3F,  //  Report Count (63)                  172
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             174
        0x85,
        0x81,  //  Report ID (129)                    176
        0x09,
        0x29,  //  Usage (Vendor Usage 0x29)          178
        0x95,
        0x3F,  //  Report Count (63)                  180
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             182
        0x85,
        0x82,  //  Report ID (130)                    184
        0x09,
        0x2A,  //  Usage (Vendor Usage 0x2a)          186
        0x95,
        0x09,  //  Report Count (9)                   188
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             190
        0x85,
        0x83,  //  Report ID (131)                    192
        0x09,
        0x2B,  //  Usage (Vendor Usage 0x2b)          194
        0x95,
        0x3F,  //  Report Count (63)                  196
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             198
        0x85,
        0x84,  //  Report ID (132)                    200
        0x09,
        0x2C,  //  Usage (Vendor Usage 0x2c)          202
        0x95,
        0x3F,  //  Report Count (63)                  204
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             206
        0x85,
        0x85,  //  Report ID (133)                    208
        0x09,
        0x2D,  //  Usage (Vendor Usage 0x2d)          210
        0x95,
        0x02,  //  Report Count (2)                   212
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             214
        0x85,
        0xA0,  //  Report ID (160)                    216
        0x09,
        0x2E,  //  Usage (Vendor Usage 0x2e)          218
        0x95,
        0x01,  //  Report Count (1)                   220
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             222
        0x85,
        0xE0,  //  Report ID (224)                    224
        0x09,
        0x2F,  //  Usage (Vendor Usage 0x2f)          226
        0x95,
        0x3F,  //  Report Count (63)                  228
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             230
        0x85,
        0xF0,  //  Report ID (240)                    232
        0x09,
        0x30,  //  Usage (Vendor Usage 0x30)          234
        0x95,
        0x3F,  //  Report Count (63)                  236
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             238
        0x85,
        0xF1,  //  Report ID (241)                    240
        0x09,
        0x31,  //  Usage (Vendor Usage 0x31)          242
        0x95,
        0x3F,  //  Report Count (63)                  244
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             246
        0x85,
        0xF2,  //  Report ID (242)                    248
        0x09,
        0x32,  //  Usage (Vendor Usage 0x32)          250
        0x95,
        0x34,  //  Report Count (52)                  252
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             254
        0x85,
        0xF4,  //  Report ID (244)                    256
        0x09,
        0x35,  //  Usage (Vendor Usage 0x35)          258
        0x95,
        0x3F,  //  Report Count (63)                  260
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             262
        0x85,
        0xF5,  //  Report ID (245)                    264
        0x09,
        0x36,  //  Usage (Vendor Usage 0x36)          266
        0x95,
        0x03,  //  Report Count (3)                   268
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             270
        0x85,
        0x60,  //  Report ID (96)                     272
        0x09,
        0x41,  //  Usage (Vendor Usage 0x41)          274
        0x95,
        0x3F,  //  Report Count (63)                  276
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             278
        0x85,
        0x61,  //  Report ID (97)                     280
        0x09,
        0x42,  //  Usage (Vendor Usage 0x42)          282
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             284
        0x85,
        0x62,  //  Report ID (98)                     286
        0x09,
        0x43,  //  Usage (Vendor Usage 0x43)          288
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             290
        0x85,
        0x63,  //  Report ID (99)                     292
        0x09,
        0x44,  //  Usage (Vendor Usage 0x44)          294
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             296
        0x85,
        0x64,  //  Report ID (100)                    298
        0x09,
        0x45,  //  Usage (Vendor Usage 0x45)          300
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             302
        0x85,
        0x65,  //  Report ID (101)                    304
        0x09,
        0x46,  //  Usage (Vendor Usage 0x46)          306
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             308
        0x85,
        0x68,  //  Report ID (104)                    310
        0x09,
        0x47,  //  Usage (Vendor Usage 0x47)          312
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             314
        0x85,
        0x70,  //  Report ID (112)                    316
        0x09,
        0x48,  //  Usage (Vendor Usage 0x48)          318
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             320
        0x85,
        0x71,  //  Report ID (113)                    322
        0x09,
        0x49,  //  Usage (Vendor Usage 0x49)          324
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             326
        0x85,
        0x72,  //  Report ID (114)                    328
        0x09,
        0x4A,  //  Usage (Vendor Usage 0x4a)          330
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             332
        0x85,
        0x73,  //  Report ID (115)                    334
        0x09,
        0x4B,  //  Usage (Vendor Usage 0x4b)          336
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             338
        0x85,
        0x74,  //  Report ID (116)                    340
        0x09,
        0x4C,  //  Usage (Vendor Usage 0x4c)          342
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             344
        0x85,
        0x75,  //  Report ID (117)                    346
        0x09,
        0x4D,  //  Usage (Vendor Usage 0x4d)          348
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             350
        0x85,
        0x76,  //  Report ID (118)                    352
        0x09,
        0x4E,  //  Usage (Vendor Usage 0x4e)          354
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             356
        0x85,
        0x77,  //  Report ID (119)                    358
        0x09,
        0x4F,  //  Usage (Vendor Usage 0x4f)          360
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             362
        0x85,
        0x78,  //  Report ID (120)                    364
        0x09,
        0x50,  //  Usage (Vendor Usage 0x50)          366
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             368
        0x85,
        0x79,  //  Report ID (121)                    370
        0x09,
        0x51,  //  Usage (Vendor Usage 0x51)          372
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             374
        0x85,
        0x7A,  //  Report ID (122)                    376
        0x09,
        0x52,  //  Usage (Vendor Usage 0x52)          378
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             380
        0x85,
        0x7B,  //  Report ID (123)                    382
        0x09,
        0x53,  //  Usage (Vendor Usage 0x53)          384
        0xB1,
        0x02,  //  Feature (Data,Var,Abs)             386
        0xC0,  // End Collection                      388
};

static unsigned char rdesc_edge_bt[] = {
        0x05,
        0x01,
        0x09,
        0x05,
        0xA1,
        0x01,
        0x85,
        0x01,
        0x09,
        0x30,
        0x09,
        0x31,
        0x09,
        0x32,
        0x09,
        0x35,
        0x15,
        0x00,
        0x26,
        0xFF,
        0x00,
        0x75,
        0x08,
        0x95,
        0x04,
        0x81,
        0x02,
        0x09,
        0x39,
        0x15,
        0x00,
        0x25,
        0x07,
        0x35,
        0x00,
        0x46,
        0x3B,
        0x01,
        0x65,
        0x14,
        0x75,
        0x04,
        0x95,
        0x01,
        0x81,
        0x42,
        0x65,
        0x00,
        0x05,
        0x09,
        0x19,
        0x01,
        0x29,
        0x0E,
        0x15,
        0x00,
        0x25,
        0x01,
        0x75,
        0x01,
        0x95,
        0x0E,
        0x81,
        0x02,
        0x75,
        0x06,
        0x95,
        0x01,
        0x81,
        0x01,
        0x05,
        0x01,
        0x09,
        0x33,
        0x09,
        0x34,
        0x15,
        0x00,
        0x26,
        0xFF,
        0x00,
        0x75,
        0x08,
        0x95,
        0x02,
        0x81,
        0x02,
        0x06,
        0x00,
        0xFF,
        0x15,
        0x00,
        0x26,
        0xFF,
        0x00,
        0x75,
        0x08,
        0x95,
        0x4D,
        0x85,
        0x31,
        0x09,
        0x31,
        0x91,
        0x02,
        0x09,
        0x3B,
        0x81,
        0x02,
        0x85,
        0x32,
        0x09,
        0x32,
        0x95,
        0x8D,
        0x91,
        0x02,
        0x85,
        0x33,
        0x09,
        0x33,
        0x95,
        0xCD,
        0x91,
        0x02,
        0x85,
        0x34,
        0x09,
        0x34,
        0x96,
        0x0D,
        0x01,
        0x91,
        0x02,
        0x85,
        0x35,
        0x09,
        0x35,
        0x96,
        0x4D,
        0x01,
        0x91,
        0x02,
        0x85,
        0x36,
        0x09,
        0x36,
        0x96,
        0x8D,
        0x01,
        0x91,
        0x02,
        0x85,
        0x37,
        0x09,
        0x37,
        0x96,
        0xCD,
        0x01,
        0x91,
        0x02,
        0x85,
        0x38,
        0x09,
        0x38,
        0x96,
        0x0D,
        0x02,
        0x91,
        0x02,
        0x85,
        0x39,
        0x09,
        0x39,
        0x96,
        0x22,
        0x02,
        0x91,
        0x02,
        0x06,
        0x80,
        0xFF,
        0x85,
        0x05,
        0x09,
        0x33,
        0x95,
        0x28,
        0xB1,
        0x02,
        0x85,
        0x08,
        0x09,
        0x34,
        0x95,
        0x2F,
        0xB1,
        0x02,
        0x85,
        0x09,
        0x09,
        0x24,
        0x95,
        0x13,
        0xB1,
        0x02,
        0x85,
        0x20,
        0x09,
        0x26,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x22,
        0x09,
        0x40,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x80,
        0x09,
        0x28,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x81,
        0x09,
        0x29,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x82,
        0x09,
        0x2A,
        0x95,
        0x09,
        0xB1,
        0x02,
        0x85,
        0x83,
        0x09,
        0x2B,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0xF1,
        0x09,
        0x31,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0xF2,
        0x09,
        0x32,
        0x95,
        0x34,
        0xB1,
        0x02,
        0x85,
        0xF0,
        0x09,
        0x30,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x60,
        0x09,
        0x41,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0x61,
        0x09,
        0x42,
        0xB1,
        0x02,
        0x85,
        0x62,
        0x09,
        0x43,
        0xB1,
        0x02,
        0x85,
        0x63,
        0x09,
        0x44,
        0xB1,
        0x02,
        0x85,
        0x64,
        0x09,
        0x45,
        0xB1,
        0x02,
        0x85,
        0x65,
        0x09,
        0x46,
        0xB1,
        0x02,
        0x85,
        0x68,
        0x09,
        0x47,
        0xB1,
        0x02,
        0x85,
        0x70,
        0x09,
        0x48,
        0xB1,
        0x02,
        0x85,
        0x71,
        0x09,
        0x49,
        0xB1,
        0x02,
        0x85,
        0x72,
        0x09,
        0x4A,
        0xB1,
        0x02,
        0x85,
        0x73,
        0x09,
        0x4B,
        0xB1,
        0x02,
        0x85,
        0x74,
        0x09,
        0x4C,
        0xB1,
        0x02,
        0x85,
        0x75,
        0x09,
        0x4D,
        0xB1,
        0x02,
        0x85,
        0x76,
        0x09,
        0x4E,
        0xB1,
        0x02,
        0x85,
        0x77,
        0x09,
        0x4F,
        0xB1,
        0x02,
        0x85,
        0x78,
        0x09,
        0x50,
        0xB1,
        0x02,
        0x85,
        0x79,
        0x09,
        0x51,
        0xB1,
        0x02,
        0x85,
        0x7A,
        0x09,
        0x52,
        0xB1,
        0x02,
        0x85,
        0x7B,
        0x09,
        0x53,
        0xB1,
        0x02,
        0x85,
        0xF4,
        0x09,
        0x2C,
        0x95,
        0x3F,
        0xB1,
        0x02,
        0x85,
        0xF5,
        0x09,
        0x2D,
        0x95,
        0x07,
        0xB1,
        0x02,
        0x85,
        0xF6,
        0x09,
        0x2E,
        0x96,
        0x22,
        0x02,
        0xB1,
        0x02,
        0x85,
        0xF7,
        0x09,
        0x2F,
        0x95,
        0x07,
        0xB1,
        0x02,
        0xC0,
        0x00,
};

static unsigned char rdesc[] = {
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
    0x95, 0x2f,                    //  Report Count (47)                  108
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
    0x95, 0x0f,                    //  Report Count (15)                  252
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             254
    0x85, 0xf4,                    //  Report ID (244)                    256
    0x09, 0x35,                    //  Usage (Vendor Usage 0x35)          258
    0x95, 0x3f,                    //  Report Count (63)                  260
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             262
    0x85, 0xf5,                    //  Report ID (245)                    264
    0x09, 0x36,                    //  Usage (Vendor Usage 0x36)          266
    0x95, 0x03,                    //  Report Count (3)                   268
    0xb1, 0x02,                    //  Feature (Data,Var,Abs)             270
    0xc0,                          // End Collection                      272
};

static unsigned char rdesc_bt[] = {
0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25,
0x07, 0x35, 0x00, 0x46, 0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0e, 0x81, 0x02,
0x75, 0x06, 0x95, 0x01, 0x81, 0x01, 0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xff,
0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x00, 0xff, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75,
0x08, 0x95, 0x4d, 0x85, 0x31, 0x09, 0x31, 0x91, 0x02, 0x09, 0x3b, 0x81, 0x02, 0x85, 0x32, 0x09,
0x32, 0x95, 0x8d, 0x91, 0x02, 0x85, 0x33, 0x09, 0x33, 0x95, 0xcd, 0x91, 0x02, 0x85, 0x34, 0x09,
0x34, 0x96, 0x0d, 0x01, 0x91, 0x02, 0x85, 0x35, 0x09, 0x35, 0x96, 0x4d, 0x01, 0x91, 0x02, 0x85,
0x36, 0x09, 0x36, 0x96, 0x8d, 0x01, 0x91, 0x02, 0x85, 0x37, 0x09, 0x37, 0x96, 0xcd, 0x01, 0x91,
0x02, 0x85, 0x38, 0x09, 0x38, 0x96, 0x0d, 0x02, 0x91, 0x02, 0x85, 0x39, 0x09, 0x39, 0x96, 0x22,
0x02, 0x91, 0x02, 0x06, 0x80, 0xff, 0x85, 0x05, 0x09, 0x33, 0x95, 0x28, 0xb1, 0x02, 0x85, 0x08,
0x09, 0x34, 0x95, 0x2f, 0xb1, 0x02, 0x85, 0x09, 0x09, 0x24, 0x95, 0x13, 0xb1, 0x02, 0x85, 0x20,
0x09, 0x26, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x22, 0x09, 0x40, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x80,
0x09, 0x28, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x81, 0x09, 0x29, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x82,
0x09, 0x2a, 0x95, 0x09, 0xb1, 0x02, 0x85, 0x83, 0x09, 0x2b, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf1,
0x09, 0x31, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2, 0x09, 0x32, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf0,
0x09, 0x30, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf4, 0x09, 0x2c, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf5,
0x09, 0x2d, 0x95, 0x07, 0xb1, 0x02, 0x85, 0xf6, 0x09, 0x2e, 0x96, 0x22, 0x02, 0xb1, 0x02, 0x85,
0xf7, 0x09, 0x2f, 0x95, 0x07, 0xb1, 0x02, 0xc0, 0x00
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

static int create(int fd, bool bluetooth, bool dualsense_edge)
{
	struct uhid_event ev;

    char uniq[sizeof(ev.u.create.uniq)];
    sprintf(uniq, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        MAC_ADDR[5], MAC_ADDR[4], MAC_ADDR[3], MAC_ADDR[2], MAC_ADDR[1], MAC_ADDR[0]
    );

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_CREATE;
    
    if (dualsense_edge) {
	    strcpy((char*)ev.u.create.name, DS5_EDGE_NAME);
    } else {
        strcpy((char*)ev.u.create.name, DS5_NAME);
    }

    if (dualsense_edge) {
        ev.u.create.rd_data = bluetooth ? rdesc_edge_bt : rdesc_edge;
        ev.u.create.rd_size = bluetooth ? sizeof(rdesc_edge_bt) : sizeof(rdesc_edge);
    } else {
        ev.u.create.rd_data = bluetooth ? rdesc_bt : rdesc;
        ev.u.create.rd_size = bluetooth ? sizeof(rdesc_bt) : sizeof(rdesc);
    }

	ev.u.create.bus = bluetooth ? BUS_BLUETOOTH : BUS_USB;
	ev.u.create.vendor = dualsense_edge ? DS5_EDGE_VENDOR : DS5_VENDOR;
	ev.u.create.product = dualsense_edge ? DS5_EDGE_PRODUCT : DS5_PRODUCT;
	ev.u.create.version = dualsense_edge ? DS5_EDGE_VERSION : DS5_VERSION;
	ev.u.create.country = 0;
    memcpy(&ev.u.create.uniq, (void*)uniq, sizeof(uniq));

	return uhid_write(fd, &ev);
}

static void destroy(int fd)
{
	struct uhid_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.type = UHID_DESTROY;

	uhid_write(fd, &ev);
}

int virt_dualsense_init(
    virt_dualsense_t *const out_gamepad,
    bool bluetooth,
    bool dualsense_edge,
    int64_t gyro_to_analog_activation_treshold,
    int64_t gyro_to_analog_mapping
) {
    int ret = 0;

    out_gamepad->gyro_to_analog_activation_treshold = absolute_value(gyro_to_analog_activation_treshold);
    out_gamepad->gyro_to_analog_mapping = gyro_to_analog_mapping;
    out_gamepad->edge_model = dualsense_edge;
    out_gamepad->bluetooth = bluetooth;
    out_gamepad->dt_sum = 0;
    out_gamepad->dt_buffer_current = 0;
    memset(out_gamepad->dt_buffer, 0, sizeof(out_gamepad->dt_buffer));
    out_gamepad->debug = false;
    out_gamepad->empty_reports = 0;
    out_gamepad->last_time = 0;
    out_gamepad->seq_num = 0;

    out_gamepad->fd = open(path, O_RDWR | O_CLOEXEC /* | O_NONBLOCK */);
    if (out_gamepad->fd < 0) {
        fprintf(stderr, "Cannot open uhid-cdev %s: %d\n", path, out_gamepad->fd);
        ret = out_gamepad->fd;
        goto virt_dualshock_init_err;
    }

    ret = create(out_gamepad->fd, bluetooth, dualsense_edge);
    if (ret) {
        fprintf(stderr, "Error creating uhid device: %d\n", ret);
        close(out_gamepad->fd);
        goto virt_dualshock_init_err;
    }

virt_dualshock_init_err:
    return ret;
}

int virt_dualsense_get_fd(virt_dualsense_t *const in_gamepad) {
    return in_gamepad->fd;
}

int virt_dualsense_event(virt_dualsense_t *const gamepad, gamepad_status_t *const out_device_status)
{
	struct uhid_event ev;
	ssize_t ret;

    int fd = virt_dualsense_get_fd(gamepad);

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
            fprintf(stderr, "UHID_CLOSE from uhid-dev\n");
        }
		break;
	case UHID_OUTPUT:
        if (gamepad->debug) {
		    printf("UHID_OUTPUT from uhid-dev\n");
        }
        // Rumble and LED messages are adverised via OUTPUT reports; ignore the rest
        if (ev.u.output.rtype != UHID_OUTPUT_REPORT)
            return 0;
        
        if (ev.u.output.size == 48) {
            fprintf(stderr, "Ignored a 48 bytes report on purpose\n");
        }

        //if (ev.u.output.size != 48)
        if (
            (!gamepad->bluetooth) && (ev.u.output.size != DS_OUTPUT_REPORT_USB_SIZE) &&
            (gamepad->bluetooth) && (ev.u.output.size != DS_OUTPUT_REPORT_BT_SIZE) 
        ) {
            fprintf(
                stderr,
                "Invalid data length: got %d, expected %d\n",
                (int)ev.u.output.size,
                (gamepad->bluetooth) ? DS_OUTPUT_REPORT_BT_SIZE : DS_OUTPUT_REPORT_USB_SIZE
            );

            return 0;
        }

        // first byte is report-id which is 0x01
        if (
            (!gamepad->bluetooth) && (ev.u.output.data[0] != DS_OUTPUT_REPORT_USB) &&
            (gamepad->bluetooth) && ((ev.u.output.data[0] != DS_OUTPUT_REPORT_BT) && (ev.u.output.data[0] < 0x10))
        ) {
            fprintf(
                stderr,
                "Unrecognised report-id: got 0x%x expected 0x%x\n",
                (int)ev.u.output.data[0],
                (gamepad->bluetooth) ? DS_OUTPUT_REPORT_BT : DS_OUTPUT_REPORT_USB
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
        // For DualShock 4 compatibility mode.
        const uint8_t motor_right = common_report[2];
        const uint8_t motor_left = common_report[3];

        // Audio controls
        const uint8_t reserved[4] = { common_report[4], common_report[5], common_report[6], common_report[7]};
        const uint8_t mute_button_led = common_report[8];

        uint8_t power_save_control = common_report[9];
        uint8_t reserved2[28];

        // LEDs and lightbar
        uint8_t valid_flag2 = common_report[38];
        uint8_t reserved3[2] = {common_report[39], common_report[40]};
        uint8_t lightbar_setup = common_report[41];
        uint8_t led_brightness = common_report[42];
        uint8_t player_leds = common_report[43];
        uint8_t lightbar_red = common_report[44];
        uint8_t lightbar_green = common_report[45];
        uint8_t lightbar_blue = common_report[46];

        if ((valid_flag0 & DS_OUTPUT_VALID_FLAG0_HAPTICS_SELECT) || ((motor_left == 0) && (motor_right == 0))) {
            uint8_t motors_shift = 0;
            if ((valid_flag2 & DS_OUTPUT_VALID_FLAG2_COMPATIBLE_VIBRATION2) || (valid_flag0 & DS_OUTPUT_VALID_FLAG0_COMPATIBLE_VIBRATION)) {
                out_device_status->motors_intensity[0] = (motor_left << 1) | ((motor_left == 0) ? 0 : 1);
                out_device_status->motors_intensity[1] = (motor_right << 1) | ((motor_right == 0) ? 0 : 1);
            } else {
                out_device_status->motors_intensity[0] = motor_left;
                out_device_status->motors_intensity[1] = motor_right;
            }

            ++out_device_status->rumble_events_count;

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

        if (valid_flag1 & DS_OUTPUT_VALID_FLAG1_LIGHTBAR_CONTROL_ENABLE) {
            out_device_status->leds_colors[0] = lightbar_red;
            out_device_status->leds_colors[1] = lightbar_green;
            out_device_status->leds_colors[2] = lightbar_blue;
            out_device_status->leds_events_count++;
        }

		break;
	case UHID_OUTPUT_EV:
        if (gamepad->debug) {
		    printf("UHID_OUTPUT_EV from uhid-dev\n");
        }
		break;
    case UHID_GET_REPORT:
        //fprintf(stderr, "UHID_GET_REPORT from uhid-dev, report=%d\n", ev.u.get_report.rnum);
        if (ev.u.get_report.rnum == DS_FEATURE_REPORT_PAIRING_INFO) {
            struct uhid_event mac_addr_response = {
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

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&mac_addr_response.u.get_report_reply.data[0], mac_addr_response.u.get_report_reply.size - 4);
                memcpy(&mac_addr_response.u.get_report_reply.data[mac_addr_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(fd, &mac_addr_response);
        } else if (ev.u.get_report.rnum == DS_FEATURE_REPORT_FIRMWARE_INFO) {
            struct uhid_event firmware_info_response = {
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

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&firmware_info_response.u.get_report_reply.data[0], firmware_info_response.u.get_report_reply.size - 4);
                memcpy(&firmware_info_response.u.get_report_reply.data[firmware_info_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(fd, &firmware_info_response);
        } else if (ev.u.get_report.rnum == DS_FEATURE_REPORT_CALIBRATION) {
            struct uhid_event calibration_response = {
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

            if (gamepad->bluetooth) {
                uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_FEATURE_CRC32_SEED, sizeof(PS_FEATURE_CRC32_SEED));
                crc = ~crc32_le(crc, (const Bytef *)&calibration_response.u.get_report_reply.data[0], calibration_response.u.get_report_reply.size - 4);
                memcpy(&calibration_response.u.get_report_reply.data[calibration_response.u.get_report_reply.size - sizeof(crc)], &crc, sizeof(crc));
            }

            uhid_write(fd, &calibration_response);
        }

		break;
	default:
		fprintf(stderr, "Invalid event from uhid-dev: %u\n", ev.type);
	}

	return 0;
}

typedef enum ds5_dpad_status {
    DPAD_N        = 0,
    DPAD_NE       = 1,
    DPAD_E        = 2,
    DPAD_SE       = 3,
    DPAD_S        = 4,
    DPAD_SW       = 5,
    DPAD_W        = 6,
    DPAD_NW       = 7,
    DPAD_RELEASED = 0x08,
} ds5_dpad_status_t;

static ds5_dpad_status_t ds5_dpad_from_gamepad(uint8_t dpad) {
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

void virt_dualsense_close(virt_dualsense_t *const out_gamepad) {
    destroy(out_gamepad->fd);
}

void virt_dualsense_compose(virt_dualsense_t *const gamepad, gamepad_status_t *const in_device_status, uint8_t *const out_buf) {
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
    const double correction_factor = DS5_SPEC_DELTA_TIME / ((double)gamepad->dt_sum / 30.f);
    if (delta_time != 0) {
        sim_time += (int)((double)delta_time * correction_factor);
    }

    const uint32_t timestamp = sim_time + (int)((double)gamepad->empty_reports * DS5_SPEC_DELTA_TIME);

    const int16_t g_x = in_device_status->raw_gyro[0];
    const int16_t g_y = in_device_status->raw_gyro[1];
    const int16_t g_z = in_device_status->raw_gyro[2];
    const int16_t a_x = in_device_status->raw_accel[0];
    const int16_t a_y = in_device_status->raw_accel[1];
    const int16_t a_z = in_device_status->raw_accel[2];

    const int64_t contrib_x = (int64_t)127 + ((int64_t)g_y / (int64_t)gamepad->gyro_to_analog_mapping);
    const int64_t contrib_y = (int64_t)127 + ((int64_t)g_x / (int64_t)gamepad->gyro_to_analog_mapping);

    out_buf[0] = gamepad->bluetooth ? DS_INPUT_REPORT_BT : DS_INPUT_REPORT_USB;  // [00] report ID (0x01)

    uint8_t *const out_shifted_buf = gamepad->bluetooth ? &out_buf[1] : &out_buf[0];
    out_shifted_buf[1] = ((uint64_t)((int64_t)in_device_status->joystick_positions[0][0] + (int64_t)32768) >> (uint64_t)8); // L stick, X axis
    out_shifted_buf[2] = ((uint64_t)((int64_t)in_device_status->joystick_positions[0][1] + (int64_t)32768) >> (uint64_t)8); // L stick, Y axis
    out_shifted_buf[3] = ((uint64_t)((int64_t)in_device_status->joystick_positions[1][0] + (int64_t)32768) >> (uint64_t)8); // R stick, X axis
    out_shifted_buf[4] = ((uint64_t)((int64_t)in_device_status->joystick_positions[1][1] + (int64_t)32768) >> (uint64_t)8); // R stick, Y axis

    if (contrib_y > gamepad->gyro_to_analog_activation_treshold) {
        if (absolute_value(contrib_x) > gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[1] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[3] - (int64_t)127) + contrib_x), 0, 255);
        }
        
        if (absolute_value(contrib_y) > gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[2] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[4] - (int64_t)127) + contrib_y), 0, 255);
        }
    }

    if (in_device_status->join_right_analog_and_gyroscope) {
        if (absolute_value(contrib_x) > gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[3] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[3] - (int64_t)127) + contrib_x), 0, 255);
        }
        
        if (absolute_value(contrib_y) > gamepad->gyro_to_analog_activation_treshold) {
            out_shifted_buf[4] = min_max_clamp((int64_t)127 + (((int64_t)out_shifted_buf[4] - (int64_t)127) + contrib_y), 0, 255);
        }
    }

    out_shifted_buf[5] = in_device_status->l2_trigger; // Z
    out_shifted_buf[6] = in_device_status->r2_trigger; // RZ
    out_shifted_buf[7] = gamepad->seq_num++; // seq_number
    out_shifted_buf[8] = (in_device_status->square ? 0x10 : 0x00) |
                (in_device_status->cross ? 0x20 : 0x00) |
                (in_device_status->circle ? 0x40 : 0x00) |
                (in_device_status->triangle ? 0x80 : 0x00) |
                (uint8_t)ds5_dpad_from_gamepad(in_device_status->dpad);
    out_shifted_buf[9] = (in_device_status->l1 ? 0x01 : 0x00) |
            (in_device_status->r1 ? 0x02 : 0x00) |
            /*(in_device_status->l2_trigger >= 225 ? 0x04 : 0x00)*/ 0x00 |
            /*(in_device_status->r2_trigger >= 225 ? 0x08 : 0x00)*/ 0x00 |
            (in_device_status->option ? 0x10 : 0x00) |
            (in_device_status->share ? 0x20 : 0x00) |
            (in_device_status->l3 ? 0x40 : 0x00) |
            (in_device_status->r3 ? 0x80 : 0x00);

    // mic button press is 0x04, touchpad press is 0x02
    out_shifted_buf[10] = ((gamepad->edge_model) && (in_device_status->l5) ? 0x40 : 0x00) |
            ((gamepad->edge_model) && (in_device_status->r5) ? 0x80 : 0x00) |
            ((gamepad->edge_model) && (in_device_status->l4) ? 0x10 : 0x00) |
            ((gamepad->edge_model) && (in_device_status->r4) ? 0x20 : 0x00) |
            (in_device_status->touchpad_press ? 0x02 : 0x00) |
            (in_device_status->center ? 0x01 : 0x00);
    //buf[11] = ;
    
    //buf[12] = 0x20; // [12] battery level | this is called sensor_temparature in the kernel driver but is never used...
    memcpy(&out_shifted_buf[16], &g_x, sizeof(int16_t));
    memcpy(&out_shifted_buf[18], &g_y, sizeof(int16_t));
    memcpy(&out_shifted_buf[20], &g_z, sizeof(int16_t));
    memcpy(&out_shifted_buf[22], &a_x, sizeof(int16_t));
    memcpy(&out_shifted_buf[24], &a_y, sizeof(int16_t));
    memcpy(&out_shifted_buf[26], &a_z, sizeof(int16_t));
    memcpy(&out_shifted_buf[28], &timestamp, sizeof(timestamp));

    // point of contact number 0
    out_shifted_buf[33] = (in_device_status->touchpad_touch_num == -1) ? 0x80 : 0x7F; //contact
    out_shifted_buf[34] = in_device_status->touchpad_x & (int16_t)0x00FF; //x_lo
    out_shifted_buf[35] = (((in_device_status->touchpad_x & (int16_t)0x0F00) >> (int16_t)8) | ((in_device_status->touchpad_y & (int16_t)0x000F) << (int16_t)4)); // x_hi:4 y_lo:4
    out_shifted_buf[36] = (in_device_status->touchpad_y & (int16_t)0x0FF0) >> (int16_t)4; //y_hi

    // point of contact number 1
    out_shifted_buf[37] = 0x80; //contact
    out_shifted_buf[38] = 0x00; //x_lo
    out_shifted_buf[39] = 0x00; //x_hi:4 y_lo:4
    out_shifted_buf[40] = 0x00; //y_hi
}

int virt_dualsense_send(virt_dualsense_t *const gamepad, uint8_t *const out_shifted_buf) {
    struct uhid_event l = {
        .type = UHID_INPUT2,
        .u = {
            .input2 = {
                .size = gamepad->bluetooth ? DS_INPUT_REPORT_BT_SIZE : DS_INPUT_REPORT_USB_SIZE,
            }
        }
    };
    

    memcpy(&l.u.input2.data[0], &out_shifted_buf[0], l.u.input2.size);

    if (gamepad->bluetooth) {
        uint32_t crc = crc32_le(0xFFFFFFFFU, (const uint8_t*)&PS_INPUT_CRC32_SEED, sizeof(PS_INPUT_CRC32_SEED));
        crc = ~crc32_le(crc, (const uint8_t *)&l.u.input2.data[0], l.u.input2.size - 4);
        memcpy(&l.u.input2.data[l.u.input2.size - sizeof(crc)], &crc, sizeof(crc));
    }

    return uhid_write(gamepad->fd, &l);
}
