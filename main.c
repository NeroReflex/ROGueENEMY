#include <signal.h>

#include "input_dev.h"
#include "output_dev.h"

ev_queue_t imu_ev;
ev_queue_t gamepad_ev;

static output_dev_t out_imu_dev = {
  .fd = -1,
  .crtl_flags = 0x00000000U,
  .queue = &imu_ev,
};

static output_dev_t out_gamepadd_dev = {
  .fd = -1,
  .crtl_flags = 0x00000000U,
  .queue = &gamepad_ev,
};

static uinput_filters_t in_asus_kb_1_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_1_dev = {
  .dev_type = input_dev_type_uinput,
  .crtl_flags = 0x00000000U,
  .ev_filters = &in_asus_kb_1_filters,
};

static uinput_filters_t in_asus_kb_2_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_2_dev = {
  .dev_type = input_dev_type_uinput,
  .crtl_flags = 0x00000000U,
  .ev_filters = &in_asus_kb_2_filters,
};

static uinput_filters_t in_asus_kb_3_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_3_dev = {
  .dev_type = input_dev_type_uinput,
  .crtl_flags = 0x00000000U,
  .ev_filters = &in_asus_kb_3_filters,
};

static uinput_filters_t in_xbox_filters = {
  .name = "Microsoft X-Box 360 pad",
};

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .crtl_flags = 0x00000000U,
  .ev_filters = &in_xbox_filters,
};

void request_termination() {
  out_imu_dev.crtl_flags |= OUTPUT_DEV_CTRL_FLAG_EXIT;
  out_gamepadd_dev.crtl_flags |= OUTPUT_DEV_CTRL_FLAG_EXIT;

  in_xbox_dev.crtl_flags |= INPUT_DEV_CTRL_FLAG_EXIT;
  in_asus_kb_3_dev.crtl_flags |= INPUT_DEV_CTRL_FLAG_EXIT;
  in_asus_kb_2_dev.crtl_flags |= INPUT_DEV_CTRL_FLAG_EXIT;
  in_asus_kb_1_dev.crtl_flags |= INPUT_DEV_CTRL_FLAG_EXIT;
}

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    request_termination();
    printf("received SIGINT\n");
  }
}

int main(int argc, char ** argv) {
  out_imu_dev.fd = create_output_dev("/dev/uinput", "Virtual IMU - ROGueENEMY", output_dev_imu);
  if (out_imu_dev.fd < 0) {
    // TODO: free(imu_dev.events_list);
    // TODO: free(gamepadd_dev.events_list);
    fprintf(stderr, "Unable to create IMU virtual device\n");
    return EXIT_FAILURE;
  }

  out_gamepadd_dev.fd = create_output_dev("/dev/uinput", "Virtual Controller - ROGueENEMY", output_dev_gamepad);
  if (out_gamepadd_dev.fd < 0) {
    close(out_imu_dev.fd);
    // TODO: free(imu_dev.events_list);
    // TODO: free(gamepadd_dev.events_list);
    fprintf(stderr, "Unable to create gamepad virtual device\n");
    return EXIT_FAILURE;
  }

  __sighandler_t sigint_hndl = signal(SIGINT, sig_handler); 
  if (sigint_hndl == SIG_ERR) {
    fprintf(stderr, "Error registering SIGINT handler\n");
    return EXIT_FAILURE;
  }

  int ret = 0;

  pthread_t imu_thread, gamepad_thread;
  pthread_t xbox_thread, asus_kb_1_thread, asus_kb_2_thread, asus_kb_3_thread;

  const int imu_thread_creation = pthread_create(&imu_thread, NULL, output_dev_thread_func, (void*)(&out_imu_dev));
  if (imu_thread_creation != 0) {
    fprintf(stderr, "Error creating IMU output thread: %d\n", imu_thread_creation);
    ret = -1;
    request_termination();
    goto imu_thread_err;
  }
  
  const int gamepad_thread_creation = pthread_create(&gamepad_thread, NULL, output_dev_thread_func, (void*)(&out_gamepadd_dev));
  if (gamepad_thread_creation != 0) {
    fprintf(stderr, "Error creating gamepad output thread: %d\n", gamepad_thread_creation);
    ret = -1;
    request_termination();
    goto gamepad_thread_err;
  }

  const int xbox_thread_creation = pthread_create(&xbox_thread, NULL, input_dev_thread_func, (void*)(&in_xbox_dev));
  if (xbox_thread_creation != 0) {
    fprintf(stderr, "Error creating xbox input thread: %d\n", xbox_thread_creation);
    ret = -1;
    request_termination();
    goto xbox_drv_thread_err;
  }

  const int asus_kb_1_thread_creation = pthread_create(&asus_kb_1_thread, NULL, input_dev_thread_func, (void*)(&in_asus_kb_1_dev));
  if (asus_kb_1_thread_creation != 0) {
    fprintf(stderr, "Error creating asus keyboard (1) input thread: %d\n", asus_kb_1_thread_creation);
    ret = -1;
    request_termination();
    goto asus_kb_1_thread_err;
  }

  const int asus_kb_2_thread_creation = pthread_create(&asus_kb_2_thread, NULL, input_dev_thread_func, (void*)(&in_asus_kb_2_dev));
  if (asus_kb_2_thread_creation != 0) {
    fprintf(stderr, "Error creating asus keyboard (2) input thread: %d\n", asus_kb_2_thread_creation);
    ret = -1;
    request_termination();
    goto asus_kb_2_thread_err;
  }

  const int asus_kb_3_thread_creation = pthread_create(&asus_kb_3_thread, NULL, input_dev_thread_func, (void*)(&in_asus_kb_3_dev));
  if (asus_kb_3_thread_creation != 0) {
    fprintf(stderr, "Error creating asus keyboard (3) input thread: %d\n", asus_kb_3_thread_creation);
    ret = -1;
    request_termination();
    goto asus_kb_3_thread_err;
  }

  pthread_join(asus_kb_3_thread, NULL);

asus_kb_3_thread_err:
  pthread_join(asus_kb_2_thread, NULL);

asus_kb_2_thread_err:
  pthread_join(asus_kb_1_thread, NULL);

asus_kb_1_thread_err:
  pthread_join(xbox_thread, NULL);

xbox_drv_thread_err:
  pthread_join(gamepad_thread, NULL);

gamepad_thread_err:
  pthread_join(imu_thread, NULL); 

imu_thread_err:
  if (!(out_gamepadd_dev.fd < 0))
    close(out_gamepadd_dev.fd);
  
  if (!(out_imu_dev.fd < 0))
    close(out_imu_dev.fd);
  
  // TODO: free(imu_dev.events_list);
  // TODO: free(gamepadd_dev.events_list);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
