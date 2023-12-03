#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <libudev.h>

#include "platform.h"

static const char* const platform_input_path = "/sys/devices/platform/asus-mcu.0/input/mode";

static struct udev_device* find_device(struct udev *udev) {
    struct udev_enumerate *const enumerate = udev_enumerate_new(udev);
    if (enumerate == NULL) {
        fprintf(stderr, "Error in udev_enumerate_new: mode switch will not be available.\n");
        return NULL;
    }

    const int add_match_subsystem_res = udev_enumerate_add_match_subsystem(enumerate, "hid");
    if (add_match_subsystem_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_subsystem: %d\n", add_match_subsystem_res);
        return NULL;
    }

    const int add_match_sysattr_res = udev_enumerate_add_match_sysattr(enumerate, "gamepad_mode", NULL);
    if (add_match_sysattr_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_sysattr: %d\n", add_match_sysattr_res);
        return NULL;
    }

    const int enumerate_scan_devices_res = udev_enumerate_scan_devices(enumerate);
    if (enumerate_scan_devices_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_scan_devices: %d\n", enumerate_scan_devices_res);
        return NULL;
    }

    struct udev_list_entry *const udev_lst_frst = udev_enumerate_get_list_entry(enumerate);

    struct udev_list_entry *list_entry = NULL;
    udev_list_entry_foreach(list_entry, udev_lst_frst) {
        const char* const name = udev_list_entry_get_name(list_entry);

        printf("Opening udev device %s...\n", name);

        struct udev_device *const device = udev_device_new_from_syspath(udev, name);
        if (device == NULL) {
            fprintf(stderr, "Could not open %s\n", name);
            continue;
        }

        // TODO: uncomment this when the rest is ready return device;
    }

    return NULL;
}

int init_platform(rc71l_platform_t *const platform) {
    platform->udev = NULL;

    if (access(platform_input_path, F_OK) != 0) {
        fprintf(stderr, "Unable to find the MCU platform mode file %s: asus-mcu not found.\n", platform_input_path);

        /* create udev object */
        platform->udev = udev_new();
        if (platform->udev == NULL) {
            fprintf(stderr, "Cannot create udev context: mode switch will not be available.\n");
            return -ENOENT;
        }

        if (find_device(platform->udev) != 0) {
            fprintf(stderr, "Cannot locate asus-mcu device: mode switch will not be available.\n");
            return -ENOENT;
        } else {
            // TODO: populate the following
            platform->modes_count = 0;
            platform->mode = 0;
            return 0;
        }

        return -ENOENT;
    }

    FILE* mode_file = fopen(platform_input_path, "r");
    if (mode_file == NULL) {
        fprintf(stderr, "Unable to open the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
        return -EINVAL;
    }

    char mode_str[12];
    unsigned long read_bytes = fread((void*)&mode_str[0], 1, sizeof(mode_str), mode_file);
    if (read_bytes < 1) {
        fprintf(stderr, "Unable to read the MCU platform mode file %s: no bytes.\n", platform_input_path);
        fclose(mode_file);
    }

    platform->mode = strtoul(&mode_str[0], NULL, 10);

    fclose(mode_file);

    printf("Asus MCU platform found: current mode %lu\n", platform->mode);
    platform->modes_count = 3;

    return 0;
}

int cycle_mode(rc71l_platform_t *const platform) {
    char new_mode_str[] = { 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };
    
    unsigned long new_mode = (platform->mode + 1) % platform->modes_count;
    sprintf(new_mode_str, "%lu\n", new_mode);

    FILE* mode_file = fopen(platform_input_path, "w");
    if (mode_file == NULL) {
        fprintf(stderr, "Unable to open the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
        return -1;
    }

    size_t len = strlen(new_mode_str);
    const int write_bytes = fwrite((void*)&new_mode_str[0], 1, len, mode_file);
    if (write_bytes < len) {
        fprintf(stderr, "Error writing new mode: expected to write %d bytes, %d written.\n", (int)len, (int)write_bytes);
        return -2;
    }

    platform->mode = new_mode;

    fclose(mode_file);

    return platform->mode;
}

int is_gamepad_mode(rc71l_platform_t *const platform) {
    return platform != NULL ? platform->mode == 0 : 0;
}

int is_mouse_mode(rc71l_platform_t *const platform) {
    return platform != NULL ? platform->mode == 1 : 0;
}

int is_macro_mode(rc71l_platform_t *const platform) {
    return platform != NULL ? platform->mode == 2 : 0;
}
