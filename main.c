#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "output_dev.h"
#include "dev_in.h"
#include "dev_out.h"
#include "logic.h"

logic_t global_logic;

static output_dev_t out_gamepadd_dev = {
  .logic = &global_logic,
};

static input_dev_t in_iio_dev = {
  .dev_type = input_dev_type_iio,
  .filters = {
    .iio = {
      .name = "bmi323",
    }
  },
  //.logic = &global_logic,
  //.input_filter_fn = input_filter_imu_identity,
};

static input_dev_t in_asus_kb_1_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  //.logic = &global_logic,
  //.ev_input_filter_fn = input_filter_asus_kb,
};

static input_dev_t in_asus_kb_2_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  //.logic = &global_logic,
  //.ev_input_filter_fn = input_filter_asus_kb,
};

static input_dev_t in_asus_kb_3_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Asus Keyboard"
    }
  },
  //.logic = &global_logic,
  //.ev_input_filter_fn = input_filter_asus_kb,
};

static uinput_filters_t in_xbox_filters = {
  .name = "Microsoft X-Box 360 pad",
};

static input_dev_t in_xbox_dev = {
  .dev_type = input_dev_type_uinput,
  .filters = {
    .ev = {
      .name = "Microsoft X-Box 360 pad"
    }
  },
  //.logic = &global_logic,
  //.ev_input_filter_fn = input_filter_asus_kb,
};

dev_in_data_t dev_in_thread_data;
dev_out_data_t dev_out_thread_data;

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    logic_request_termination(&global_logic);
    printf("Received SIGINT\n");
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

  int out_message_pipes[2];
  pipe(out_message_pipes);

  int in_message_pipes[2];
  pipe(in_message_pipes);

  // populate the input device thread data
  dev_in_thread_data.timeout_ms = 400;
  dev_in_thread_data.in_message_pipe_fd = in_message_pipes[1];
  dev_in_thread_data.out_message_pipe_fd = out_message_pipes[0];
  dev_in_thread_data.input_dev_decl = *in_devs;
  dev_in_thread_data.input_dev_cnt = sizeof(in_devs) / sizeof(input_dev_t *);
  
  // populate the output device thread data
  //dev_out_thread_data.timeout_ms = 400;
  dev_out_thread_data.in_message_pipe_fd = in_message_pipes[0];
  dev_out_thread_data.out_message_pipe_fd = out_message_pipes[1];

  pthread_t dev_in_thread;
  const int dev_in_thread_creation = pthread_create(&dev_in_thread, NULL, dev_in_thread_func, (void*)(&dev_in_thread_data));
  if (dev_in_thread_creation != 0) {
    fprintf(stderr, "Error creating dev_in thread: %d\n", dev_in_thread_creation);
    ret = -1;
    logic_request_termination(&global_logic);
    goto main_err;
  }

  pthread_t dev_out_thread;
  const int dev_out_thread_creation = pthread_create(&dev_out_thread, NULL, dev_out_thread_func, (void*)(&dev_out_thread_data));
  if (dev_out_thread_creation != 0) {
    fprintf(stderr, "Error creating dev_out thread: %d\n", dev_out_thread_creation);
    ret = -1;
    logic_request_termination(&global_logic);
    goto main_err;
  }

/*
  // TODO: once the application is able to exit de-comment this
  __sighandler_t sigint_hndl = signal(SIGINT, sig_handler); 
  if (sigint_hndl == SIG_ERR) {
    fprintf(stderr, "Error registering SIGINT handler\n");
    return EXIT_FAILURE;
  }
*/

main_err:
  if (dev_in_thread_creation == 0) {
    pthread_join(dev_in_thread, NULL);
  }

  if (dev_out_thread_creation == 0) {
    pthread_join(dev_out_thread, NULL);
  }

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
