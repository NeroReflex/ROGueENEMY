#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "dev_in.h"
#include "dev_out.h"
#include "ipc.h"
#include "settings.h"

#include "rog_ally.h"
#include "legion_go.h"

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

int main(int argc, char ** argv) {
  int ret = 0;

  // fill in configuration from file: automatic fallback to default
  dev_in_settings_t in_settings = {
    .enable_qam = true,
    .ff_gain = 0xFFFF,
    .rumble_on_mode_switch = true,
    .m1m2_mode = 0,
    .touchbar = true,
    .enable_thermal_profiles_switching = false,
    .default_thermal_profile = -1,
    .enable_leds_commands = false,
    .enable_imu = true,
    .imu_polling_interface = true,
  };
  
  load_in_config(&in_settings, configuration_file);

  dev_out_settings_t out_settings = {
    .default_gamepad = 0,
    .nintendo_layout = false,
    .gamepad_leds_control = true,
    .gamepad_rumble_control = true,
    .controller_bluetooth = false,
    .dualsense_edge = false,
    .swap_y_z = false,
    .gyro_to_analog_activation_treshold = 16,
    .gyro_to_analog_mapping = 4,
  };

  load_out_config(&out_settings, configuration_file);

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
    in_devs = rog_ally_device_def(&in_settings);
  } else if (strstr(bname, "LNVNB161216")) {
    printf("Running in an Lenovo Legion Go device\n");
    in_devs = legion_go_device_def();
  }
  close(dmi_name_fd);

  int dev_in_thread_creation = -1;
  int dev_out_thread_creation = -1;

  int out_message_pipes[2];
  const int out_msg_pipe_res = pipe(out_message_pipes);
  if (out_msg_pipe_res != 0) {
    fprintf(stderr, "Unable to create out_message pipe: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  int in_message_pipes[2];
  const int in_msg_pipe_res = pipe(in_message_pipes);
  if (in_msg_pipe_res != 0) {
    fprintf(stderr, "Unable to create out_message pipe: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  // Create a signal set containing only SIGTERM
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGINT);

  // Block SIGTERM for the current thread
  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
      perror("sigprocmask");
      exit(EXIT_FAILURE);
  }

  // Create a signalfd for the specified signals
  const int sfd = signalfd(-1, &mask, 0);
  if (sfd == -1) {
      perror("signalfd");
      exit(EXIT_FAILURE);
  }

  // populate the input device thread data
  dev_in_data_t dev_in_thread_data = {
    .timeout_ms = 800,
    .input_dev_decl = in_devs,
    .flags = 0x00000000U,
    .communication = {
      .type = ipc_unix_pipe,
      .endpoint = {
        .pipe = {
          .in_message_pipe_fd = in_message_pipes[1],
          .out_message_pipe_fd = out_message_pipes[0],
        }
      }
    },
    .settings = in_settings,
  };

  // populate the output device thread data
  dev_out_data_t dev_out_thread_data = {
    .flags = 0x00000000U,
    .communication = {
      .type = ipc_unix_pipe,
      .endpoint = {
        .pipe = {
          .in_message_pipe_fd = in_message_pipes[0],
          .out_message_pipe_fd = out_message_pipes[1],
        }
      }
    },
    .settings = out_settings,
  };

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

  struct pollfd sigpoll = {
    .fd = sfd,
    .events = POLL_IN,
  };

  for (;;) {
    sigpoll.revents = 0;

    poll(&sigpoll, 1, (dev_in_thread_data.timeout_ms / 2) - 1);
 
    if (sigpoll.revents & POLL_IN) {
      // Read signals from the signalfd
      struct signalfd_siginfo si;
      ssize_t s = read(sfd, &si, sizeof(struct signalfd_siginfo));

      if (s != sizeof(struct signalfd_siginfo)) {
          perror("Error reading signalfd\n");
          exit(EXIT_FAILURE);
      }

      // Check the signal received
      if (si.ssi_signo == SIGTERM) {
        printf("Received SIGTERM -- propagating signal\n");
        dev_in_thread_data.flags |= DEV_IN_FLAG_EXIT;
        dev_out_thread_data.flags |= DEV_OUT_FLAG_EXIT;
        goto main_exit;
      } else if (si.ssi_signo == SIGINT) {
        printf("Received SIGINT -- propagating signal\n");
        dev_in_thread_data.flags |= DEV_IN_FLAG_EXIT;
        dev_out_thread_data.flags |= DEV_OUT_FLAG_EXIT;
        goto main_exit;
      }
    }
  }

main_exit:
main_err:
  if (dev_in_thread_creation == 0) {
    pthread_join(dev_in_thread, NULL);
    printf("dev_in_thread terminated\n");
  }

  if (dev_out_thread_creation == 0) {
    pthread_join(dev_out_thread, NULL);
    printf("dev_out_thread terminated\n");
  }

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
