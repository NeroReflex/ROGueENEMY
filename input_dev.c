#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "input_dev.h"

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