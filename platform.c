#include <asm-generic/errno-base.h>
#include <stdlib.h>
#define PLATFORM_FILE
#include "platform.h"

static const char* const platform_input_path = "/sys/devices/platform/asus-mcu.0/input/mode";

static rc71l_platform_t static_platform;

void init_global_mode() {
    global_platform = NULL;

    if (access(platform_input_path, F_OK) == 0) {
        FILE* mode_file = fopen(platform_input_path, "r");
        if (mode_file == NULL) {
            fprintf(stderr, "Unable to open the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
            return;
        }

        char mode_str[12];
        unsigned long read_bytes = fread((void*)&mode_str[0], 1, sizeof(mode_str), mode_file);
        if (read_bytes < 1) {
            fprintf(stderr, "Unable to read the MCU platform mode file %s: no bytes.\n", platform_input_path);
            fclose(mode_file);
        }

        static_platform.mode = strtoul(&mode_str[0], NULL, 10);

        fclose(mode_file);

        global_platform = &static_platform;
        printf("Asus MCU platform found: current mode %lu\n", global_platform->mode);
        global_platform->modes_count = 3;
    } else {
        fprintf(stderr, "Unable to find the MCU platform mode file %s: modes cannot be switched.\n", platform_input_path);
    }
}

int cycle_mode() {
    if (global_platform == NULL) {
        fprintf(stderr, "Asus MCU not registered: aborting.\n");
    }

    char new_mode_str[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned long new_mode = (global_platform->mode + 1) % global_platform->modes_count;
    sprintf(new_mode_str, "%lu", new_mode);

    FILE* mode_file = fopen(platform_input_path, "r");
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

    fclose(mode_file);

    return 0;
}
