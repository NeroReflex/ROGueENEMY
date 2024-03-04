#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "dev_in.h"
#include "dev_out.h"
#include "ipc.h"
#include "settings.h"

#include "rog_ally.h"
#include "legion_go.h"

#include <sys/mman.h>

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

int main(int argc, char ** argv) {
  // Lock all current and future pages from preventing of being paged to swap
  const int lockall_res = mlockall( MCL_CURRENT | MCL_FUTURE );
  if (lockall_res) { 
    fprintf(stderr, "mlockall failed: %d", lockall_res);
  }

  int ret = 0;

  dev_in_settings_t in_settings = {
    .enable_qam = true,
    .ff_gain = 0xFFFF,
    .rumble_on_mode_switch = true,
    .m1m2_mode = 1,
    .touchbar = true,
    .enable_thermal_profiles_switching = false,
    .default_thermal_profile = -1,
    .enable_leds_commands = false,
    .enable_imu = true,
    .imu_polling_interface = true,
  };
  
  load_in_config(&in_settings, configuration_file);

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

  // populate the input device thread data
  dev_in_data_t dev_in_thread_data = {
    .timeout_ms = 800,
    .input_dev_decl = in_devs,
    .communication = {
      .type = ipc_client_socket,
      .endpoint = {
        .socket = {
          .fd = -1,
          .serveraddr = {
            .sun_path = SERVER_PATH,
            .sun_family = AF_UNIX,
          }
        },
      }
    },
    .settings = in_settings,
  };

  // fill in configuration from file: automatic fallback to default
  load_in_config(&dev_in_thread_data.settings, configuration_file);

  //memset(&dev_in_thread_data.communication.endpoint.socket.serveraddr, 0, sizeof(dev_in_thread_data.communication.endpoint.socket.serveraddr));
  
  // Initialize pthread attributes (default values)
  struct sched_param param;
  pthread_attr_t attr;
  ret = pthread_attr_init(&attr);
  if (ret) {
          printf("init pthread attributes failed\n");
          goto main_err;
  }

  // Set a specific stack size
  ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + 8192);
  if (ret) {
      printf("pthread setstacksize failed\n");
      goto main_err;
  }

  // Set scheduler policy and priority of pthread
  ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  if (ret) {
      printf("pthread setschedpolicy failed\n");
      goto main_err;
  }
  param.sched_priority = 80;
  ret = pthread_attr_setschedparam(&attr, &param);
  if (ret) {
      printf("pthread setschedparam failed\n");
      goto main_err;
  }

  // Use scheduling parameters of attr
  ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  if (ret) {
      printf("pthread setinheritsched failed\n");
      goto main_err;
  }


  pthread_t dev_in_thread;
  const int dev_in_thread_creation = pthread_create(&dev_in_thread, &attr, dev_in_thread_func, (void*)(&dev_in_thread_data));
  if (dev_in_thread_creation != 0) {
    fprintf(stderr, "Error creating dev_in thread: %d\n", dev_in_thread_creation);
    ret = -1;
    //logic_request_termination(&global_logic);
    goto main_err;
  }

main_err:
  if (dev_in_thread_creation == 0) {
    pthread_join(dev_in_thread, NULL);
  }

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
