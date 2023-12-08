#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <libudev.h>

#include <dirent.h>

#include "platform.h"

static const char* const platform_input_path = "/sys/devices/platform/asus-mcu.0/input/mode";

static int hidraw_cycle_to_mode(const char* const path, int controller_mode) {
    int res = 0;

    if ((controller_mode < 0) || (controller_mode > 2)) {
        res = -EINVAL;
        goto hidraw_cycle_to_mode_err_mode; 
    }

    const char* hidraw_subdir = "/hidraw/";

    const unsigned long len = strlen(path) + strlen(hidraw_subdir) + 64;
    char* hidraw_path = malloc(len + 1);
    if (hidraw_path == NULL) {
        res = -ENOMEM;
        goto hidraw_cycle_to_mode_err_path;
    }
    
    memset(hidraw_path, 0, len + 1);
    strcat(hidraw_path, path);
    strcat(hidraw_path, hidraw_subdir);
    
    DIR *d;
    struct dirent *dir;
    d = opendir(hidraw_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, "hidraw") == NULL) { // h as in hidraw
                continue;
            }

            memset(hidraw_path, 0, len + 1);
            strcat(hidraw_path, "/dev/");
            strcat(hidraw_path, dir->d_name);

            //strcat(hidraw_path, dir->d_name);
            //strcat(hidraw_path, "/dev");

            printf("Using hidraw located at: %s\n", hidraw_path);

            int fd = open(hidraw_path, O_RDWR);
            if (fd == -1) {
                fprintf(stderr, "Error opening hidraw device %s\n", hidraw_path);

                res = -EIO;

                goto hidraw_cycle_to_mode_err;
            }

            for (int i = 0; i < 23; ++i) {
                const int write_res = write(fd, &rc71l_mode_switch_commands[controller_mode][i][0], 64);
                if (write_res != 64) {
                    fprintf(stderr, "Error writing packet %d/23: %d bytes sent, 64 expected\n", i, write_res);
                    break;
                }
            }

            close(fd);

            if (res == 0) {
                printf("Control messages sent successfully.\n");
            } else {
                goto hidraw_cycle_to_mode_err;
            }
            
        }
    }



hidraw_cycle_to_mode_err:
    free(hidraw_path);

hidraw_cycle_to_mode_err_path:
hidraw_cycle_to_mode_err_mode:
    return res;
}

static char* find_device(struct udev *udev) {
    struct udev_enumerate *const enumerate = udev_enumerate_new(udev);
    if (enumerate == NULL) {
        fprintf(stderr, "Error in udev_enumerate_new: mode switch will not be available.\n");
        return NULL;
    }

    const int add_match_subsystem_res = udev_enumerate_add_match_subsystem(enumerate, "hid");
    if (add_match_subsystem_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_subsystem: %d\n", add_match_subsystem_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    const int add_match_sysattr_res = udev_enumerate_add_match_sysattr(enumerate, "gamepad_mode", NULL);
    if (add_match_sysattr_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_add_match_sysattr: %d\n", add_match_sysattr_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    const int enumerate_scan_devices_res = udev_enumerate_scan_devices(enumerate);
    if (enumerate_scan_devices_res != 0) {
        fprintf(stderr, "Error in udev_enumerate_scan_devices: %d\n", enumerate_scan_devices_res);

        udev_enumerate_unref(enumerate);

        return NULL;
    }

    struct udev_list_entry *const udev_lst_frst = udev_enumerate_get_list_entry(enumerate);

    struct udev_list_entry *list_entry = NULL;
    udev_list_entry_foreach(list_entry, udev_lst_frst) {
        const char* const name = udev_list_entry_get_name(list_entry);

        const unsigned long len = strlen(name) + 1;
        char *const result = malloc(len);
        memset(result, 0, len);
        strncat(result, name, len - 1);

        udev_enumerate_unref(enumerate);

        return result;
    }

    udev_enumerate_unref(enumerate);
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
            platform->mode = -1;
            return -ENOENT;
        }

        char *const dev_name = find_device(platform->udev);
        if (dev_name == NULL) {
            fprintf(stderr, "Cannot locate asus-mcu device: mode switch will not be available.\n");
            platform->mode = -1;
            return -ENOENT;
        } else {
            printf("Asus MCU over hidraw: %s -- mode will be reset\n", dev_name);

            platform->platform_mode = rc71l_platform_mode_hidraw;
            platform->modes_count = 2;
            platform->mode = 0;

            // reset to mode 0: game mode
            const int reset_res = hidraw_cycle_to_mode(dev_name, 0);
            if (reset_res != 0) {
                fprintf(stderr, "Unable to reset Asus MCU over hidraw: %d -- Asus MCU will be unavailable.\n", reset_res);
                
                free(dev_name);

                return -EIO;
            }

            // find_device does malloc
            free(dev_name);

            return 0;
        }

        return -ENOENT;
    }

    FILE* mode_file = fopen(platform_input_path, "r");
    if (mode_file == NULL) {
        fprintf(stderr, "Unable to open the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
        platform->mode = -1;
        return -EINVAL;
    }

    char mode_str[12];
    unsigned long read_bytes = fread((void*)&mode_str[0], 1, sizeof(mode_str), mode_file);
    if (read_bytes < 1) {
        fprintf(stderr, "Unable to read the MCU platform mode file %s: no bytes.\n", platform_input_path);
        fclose(mode_file);
        platform->mode = -1;
        return -EINVAL;
    }

    platform->mode = strtoul(&mode_str[0], NULL, 10);

    fclose(mode_file);

    printf("Asus MCU platform found: current mode %lu\n", platform->mode);
    platform->modes_count = 2;

    platform->platform_mode = rc71l_platform_mode_asus_mcu;

    return 0;
}

int cycle_mode(rc71l_platform_t *const platform) {
    if (platform == NULL) {
        return -ENOENT;
    }

    char new_mode_str[] = { 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };
    
    unsigned long new_mode = (platform->mode + 1) % platform->modes_count;
    sprintf(new_mode_str, "%lu\n", new_mode);

    if (platform->platform_mode == rc71l_platform_mode_hidraw) {
        char *const dev_name = find_device(platform->udev);
        if (dev_name == NULL) {
            fprintf(stderr, "Unable to locate Asus MCU hidraw to mode-switch. Mode will not be switched.\n");
            return -ENOENT;
        }

        const int next_mode = (platform->mode + 1) % platform->modes_count;

        const int reset_res = hidraw_cycle_to_mode(dev_name, next_mode);
        if (reset_res != 0) {
            fprintf(stderr, "Unable to change mode of Asus MCU over hidraw: %d.\n", reset_res);
		}

		free(dev_name);

		platform->mode = next_mode;

		printf("Used hidraw to switch Asus MCU to mode %lu\n", platform->mode);
		
        return reset_res;
    } else if (platform->platform_mode == rc71l_platform_mode_asus_mcu) {
        FILE* mode_file = fopen(platform_input_path, "w");
        if (mode_file == NULL) {
            fprintf(stderr, "Unable to open the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
            return -1;
        }

        size_t len = strlen(new_mode_str);
        const int write_bytes = fwrite((void*)&new_mode_str[0], 1, len, mode_file);
        if (write_bytes < len) {
            fprintf(stderr, "Error writing new mode: expected to write %d bytes, %d written.\n", (int)len, (int)write_bytes);
            return -EIO;
        }

        platform->mode = new_mode;

		printf("Used asus-mcu to switch Asus MCU to mode %lu\n", platform->mode);

        fclose(mode_file);
    }

    return -ENOENT;
}

int is_mouse_mode(rc71l_platform_t *const platform) {
    return platform != NULL ? platform->mode == 1 : 0;
}
