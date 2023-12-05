#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "output_dev.h"
#include "logic.h"

logic_t global_logic;

static output_dev_t out_gamepadd_dev = {
  .logic = &global_logic,
};

static iio_filters_t in_iio_filters = {
  .name = "bmi323",
};

static input_dev_t in_iio_dev = {
  .dev_type = input_dev_type_iio,
  .iio_filters = &in_iio_filters,
  .logic = &global_logic,
  //.input_filter_fn = input_filter_imu_identity,
};

static uinput_filters_t in_asus_kb_1_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_1_dev = {
  .dev_type = input_dev_type_uinput,
  .ev_filters = &in_asus_kb_1_filters,
  .logic = &global_logic,
  .ev_input_filter_fn = input_filter_asus_kb,
};

static uinput_filters_t in_asus_kb_2_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_2_dev = {
  .dev_type = input_dev_type_uinput,
  .ev_filters = &in_asus_kb_2_filters,
  .logic = &global_logic,
  .ev_input_filter_fn = input_filter_asus_kb,
};

static uinput_filters_t in_asus_kb_3_filters = {
  .name = "Asus Keyboard",
};

static input_dev_t in_asus_kb_3_dev = {
  .dev_type = input_dev_type_uinput,
  .ev_filters = &in_asus_kb_3_filters,
  .logic = &global_logic,
  .ev_input_filter_fn = input_filter_asus_kb,
};

static uinput_filters_t in_xbox_filters = {
  .name = "Microsoft X-Box 360 pad",
};

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .ev_filters = &in_xbox_filters,
  .logic = &global_logic,
  .ev_input_filter_fn = input_filter_identity,
};

int start_input_devices(pthread_t *out_threads, input_dev_t **in_devs, uint16_t in_count) {
  int ret = 0;

  uint16_t i = 0;
  for (i = 0; i < in_count; ++i) {
    const int thread_creation = pthread_create(&out_threads[i], NULL, input_dev_thread_func, (void*)(in_devs[i]));
    if (thread_creation != 0) {
      fprintf(stderr, "Error creating input thread for device %d: %d\n", (int)i, thread_creation);
      ret = -1;
      // TODO: logic_request_termination(&global_logic);
      goto start_input_devices_err;
    }
  }

start_input_devices_err:
  return ret;
}

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    logic_request_termination(&global_logic);
    printf("received SIGINT\n");
  }
}

int main(int argc, char ** argv) {
  int ret = 0;

  const int logic_creation_res = logic_create(&global_logic);
  if (logic_creation_res < 0) {
    fprintf(stderr, "Unable to create logic: %d", logic_creation_res);
    return EXIT_FAILURE;
  }

  
  input_dev_t *in_devs[] = {
    &in_xbox_dev,
    &in_iio_dev,
    &in_asus_kb_1_dev,
    &in_asus_kb_2_dev,
    &in_asus_kb_3_dev,
  };

  const uint16_t input_dev_count = (sizeof(in_devs) / sizeof(input_dev_t *));

  pthread_t *out_threads = malloc(sizeof(pthread_t) * input_dev_count);

  ret = start_input_devices(out_threads, in_devs, input_dev_count);
  if (ret != 0) {
    fprintf(stderr, "Unable to start input devices: %d -- terminating...\n", ret);
    goto input_threads_err;
  }

  pthread_t gamepad_thread;

  const int gamepad_thread_creation = pthread_create(&gamepad_thread, NULL, output_dev_thread_func, (void*)(&out_gamepadd_dev));
  if (gamepad_thread_creation != 0) {
    fprintf(stderr, "Error creating gamepad output thread: %d\n", gamepad_thread_creation);
    ret = -1;
    logic_request_termination(&global_logic);
    goto gamepad_thread_err;
  }

/*
  // TODO: once the application is able to exit de-comment this
  __sighandler_t sigint_hndl = signal(SIGINT, sig_handler); 
  if (sigint_hndl == SIG_ERR) {
    fprintf(stderr, "Error registering SIGINT handler\n");
    return EXIT_FAILURE;
  }
*/

  for (uint16_t j = 0; j < input_dev_count; ++j) {
    pthread_join(out_threads[j], NULL);
  }

  pthread_join(gamepad_thread, NULL);

gamepad_thread_err:
input_threads_err:
  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
