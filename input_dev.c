#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "input_dev.h"

#include <dirent.h> 
#include <stdio.h> 

pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

void *input_dev_thread_func(void *ptr) {
    input_dev_t *out_dev = (input_dev_t*)ptr;

    for (;;) {
        const uint32_t flags = out_dev->crtl_flags;
        if (flags & INPUT_DEV_CTRL_FLAG_EXIT) {
            out_dev->crtl_flags &= ~INPUT_DEV_CTRL_FLAG_EXIT;
            break;
        }
        
        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            continue;
        }

        char path[512] = "\0";
        const char *input_path = "/sys/class/input/";

        DIR *d;
        struct dirent *dir;
        d = opendir(input_path);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.') {
                    continue;
                } 

                sprintf(path, "%s%s", input_path, dir->d_name);

                printf("device: %s\n", path);

                char name[510] = "\0";

                char name_path[512] = "\0";
                sprintf(name_path, "%s/device/name", path);

                FILE* n = fopen(name_path, "r");
                if (n == NULL) {
                    fprintf(stderr, "    name: [ERROR]: %s\n", name_path);
                    continue;
                }

                fseek(n, 0L, SEEK_END);
                const size_t name_length = ftell(n);
                fseek(n, 0L, SEEK_SET);
                fread(name, 1, name_length, n);
                fclose(n);
                printf("    name: %s\n", name);
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        for (;;) {
            // TODO: do the required

            const uint32_t flags = out_dev->crtl_flags;
            if (flags & INPUT_DEV_CTRL_FLAG_EXIT) {
                break;
            }
        }
    }

    return NULL;
}


int open_and_hide_input() {
    int fd = -1;
    char buf[256];

    fd = open("/dev/input/event3", O_RDONLY);
    if (fd == -1) {
        perror("open_port: Unable to open /dev/ttyAMA0 - ");
        return(-1);
    }

    // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
    //fcntl(fd, F_SETFL, 0);

    while(1){
        int n = read(fd, (void*)buf, 255);
        if (n < 0) {
            perror("Read failed - ");
        return -1;
        } else if (n == 0) {
            printf("No data on port\n");
        } else {
            buf[n] = '\0';
            printf("%i bytes read : %s", n, buf);
        }
        sleep(1);
        printf("i'm still doing something");
    }
    close(fd);
    return fd;
}
