#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "dev_in.h"
#include "dev_out.h"
#include "settings.h"

#include "rog_ally.h"
#include "legion_go.h"

/*
logic_t global_logic;

static output_dev_t out_gamepadd_dev = {
  .logic = &global_logic,
};

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    logic_request_termination(&global_logic);
    printf("Received SIGINT\n");
  }
}
*/

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

dev_in_data_t dev_in_thread_data;
dev_out_data_t dev_out_thread_data;

controller_settings_t settings;

int main(int argc, char ** argv) {
  int ret = 0;

  init_config(&settings);
  fill_config(&settings, configuration_file);

/*
  const int logic_creation_res = logic_create(&global_logic);
  if (logic_creation_res < 0) {
    fprintf(stderr, "Unable to create logic: %d", logic_creation_res);
    return EXIT_FAILURE;
  }
*/
  input_dev_composite_t* in_devs = NULL;
  
  int dmi_name_fd = open("/sys/class/dmi/id/board_name", O_RDONLY | O_NONBLOCK);
  if (dmi_name_fd < 0) {
    fprintf(stderr, "Cannot get the board name\n");
    return EXIT_FAILURE;
  }

  char bname[256];
  memset(bname, 0, sizeof(bname));
  read(dmi_name_fd, bname, sizeof(bname));
  if (strstr(bname, "RC71L") != NULL) {
    printf("Running in an Asus ROG Ally device\n");
    in_devs = rog_ally_device_def(&settings);
  } else if (strstr(bname, "LNVNB161216")) {
    printf("Running in an Lenovo Legion Go device\n");
    in_devs = legion_go_device_def(&settings);
  }
  close(dmi_name_fd);

  int dev_in_thread_creation = -1;
  int dev_out_thread_creation = -1;

  int out_message_pipes[2];
  pipe(out_message_pipes);

  int in_message_pipes[2];
  pipe(in_message_pipes);

  // populate the input device thread data
  dev_in_thread_data.timeout_ms = 400;
  dev_in_thread_data.in_message_pipe_fd = in_message_pipes[1];
  dev_in_thread_data.out_message_pipe_fd = out_message_pipes[0];
  dev_in_thread_data.input_dev_decl = in_devs;
  
  // populate the output device thread data
  //dev_out_thread_data.timeout_ms = 400;
  dev_out_thread_data.in_message_pipe_fd = in_message_pipes[0];
  dev_out_thread_data.out_message_pipe_fd = out_message_pipes[1];
  dev_out_thread_data.gamepad = GAMEPAD_DUALSENSE;

  pthread_t dev_in_thread;
  dev_in_thread_creation = pthread_create(&dev_in_thread, NULL, dev_in_thread_func, (void*)(&dev_in_thread_data));
  if (dev_in_thread_creation != 0) {
    fprintf(stderr, "Error creating dev_in thread: %d\n", dev_in_thread_creation);
    ret = -1;
    //logic_request_termination(&global_logic);
    goto main_err;
  }

  pthread_t dev_out_thread;
  dev_out_thread_creation = pthread_create(&dev_out_thread, NULL, dev_out_thread_func, (void*)(&dev_out_thread_data));
  if (dev_out_thread_creation != 0) {
    fprintf(stderr, "Error creating dev_out thread: %d\n", dev_out_thread_creation);
    ret = -1;
    //logic_request_termination(&global_logic);
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
