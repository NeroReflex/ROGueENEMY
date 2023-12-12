#include <pthread.h>
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

controller_settings_t settings;

int main(int argc, char ** argv) {
    int ret = 0;

    init_config(&settings);
    fill_config(&settings, configuration_file);

    // populate the output device thread data
    dev_out_data_t dev_out_thread_data = {
        .communication = {
            .type = ipc_server_sockets,
            .endpoint = {
                .ssocket = {
                    .mutex = PTHREAD_MUTEX_INITIALIZER,
                    .clients = { -1, -1, -1, -1, -1, -1, -1, -1 },
                }
            }
        },
        .gamepad = GAMEPAD_DUALSENSE,
    };

    pthread_t dev_out_thread;
    const int dev_out_thread_creation = pthread_create(&dev_out_thread, NULL, dev_out_thread_func, (void*)(&dev_out_thread_data));
    if (dev_out_thread_creation != 0) {
        fprintf(stderr, "Error creating dev_out thread: %d\n", dev_out_thread_creation);
        ret = -1;
        //logic_request_termination(&global_logic);
        goto main_err;
    }

    int    sd=-1;
    int    rc, length;
    struct sockaddr_un serveraddr;
    do {
        sd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sd < 0)
        {
            fprintf(stderr, "socket() failed");
            break;
        }

        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sun_family = AF_UNIX;
        strcpy(serveraddr.sun_path, SERVER_PATH);

        int rc = bind(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
        if (rc < 0)
        {
            perror("bind() failed");
            break;
        }

        while (true) {
            const int client_fd = accept(sd, NULL, NULL);
            if (client_fd < 0) {
                fprintf(stderr, "Error in getting a client connected: %d\n", client_fd);
                continue;
            }

            // here the client_fd is good
            if (pthread_mutex_lock(&dev_out_thread_data.communication.endpoint.ssocket.mutex) == 0) {
                int i;
                for (i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                    if (dev_out_thread_data.communication.endpoint.ssocket.clients[i] == -1) {
                        dev_out_thread_data.communication.endpoint.ssocket.clients[i] = client_fd;
                        break;
                    }
                }

                if (i == MAX_CONNECTED_CLIENTS) {
                    fprintf(stderr, "Could not find a free spot fot the incoming client -- client will be rejected\n");
                    close(client_fd);
                }

                pthread_mutex_unlock(&dev_out_thread_data.communication.endpoint.ssocket.mutex);
            }
        }
    } while (false);
        
    if (sd != -1) {
        close(sd);
    }

    unlink(SERVER_PATH);

/*
  // TODO: once the application is able to exit de-comment this
  __sighandler_t sigint_hndl = signal(SIGINT, sig_handler); 
  if (sigint_hndl == SIG_ERR) {
    fprintf(stderr, "Error registering SIGINT handler\n");
    return EXIT_FAILURE;
  }
*/

main_err:
    if (dev_out_thread_creation == 0) {
        pthread_join(dev_out_thread, NULL);
    }

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
