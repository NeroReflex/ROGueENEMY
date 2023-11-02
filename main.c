#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "imu_output.h"
#include "gamepad_output.h"

output_dev_t imu_dev = {
  .fd = -1,
  .ctrl_mutex = PTHREAD_MUTEX_INITIALIZER,
  .crtl_flags = 0x00000000U,
};

output_dev_t gamepadd_dev = {
  .fd = -1,
  .ctrl_mutex = PTHREAD_MUTEX_INITIALIZER,
  .crtl_flags = 0x00000000U,
};

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    pthread_mutex_lock(&imu_dev.ctrl_mutex);
    imu_dev.crtl_flags |= OUTPUT_DEV_CTRL_FLAG_EXIT;
    pthread_mutex_unlock(&imu_dev.ctrl_mutex);

    pthread_mutex_lock(&gamepadd_dev.ctrl_mutex);
    gamepadd_dev.crtl_flags |= OUTPUT_DEV_CTRL_FLAG_EXIT;
    pthread_mutex_unlock(&gamepadd_dev.ctrl_mutex);
  
    printf("received SIGINT\n");
  }
}

int main(int argc, char ** argv) {
  
  __sighandler_t sigint_hndl = signal(SIGINT, sig_handler); 
  if (sigint_hndl == SIG_ERR) {
    fprintf(stderr, "Error registering SIGINT handler");
    return -1;
  }

  int ret = 0;

  pthread_t imu_thread, gamepad_thread;

  int imu_thread_creation = pthread_create(&imu_thread, NULL, imu_thread_func, (void*)(&imu_dev));
  if (imu_thread_creation != 0) {
    fprintf(stderr, "Error creating IMU output thread: %d", imu_thread_creation);
    ret = -1;
    goto imu_thread_err;
  }
  
  int gamepad_thread_creation = pthread_create(&gamepad_thread, NULL, gamepad_thread_func, (void*)(&gamepadd_dev));
  if (gamepad_thread_creation != 0) {
    fprintf(stderr, "Error creating gamepad output thread: %d", gamepad_thread_creation);
    ret = -1;
    goto gamepad_thread_err;
  }

  pthread_join(gamepad_thread, NULL);

gamepad_thread_err:
  pthread_join(imu_thread, NULL); 

imu_thread_err:
  return ret;

  int fd;
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
  return 0;
}