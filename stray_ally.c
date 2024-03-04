#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "input_dev.h"
#include "dev_in.h"
#include "dev_out.h"
#include "settings.h"

#include <sys/mman.h>

static const char* configuration_file = "/etc/ROGueENEMY/config.cfg";

int main(int argc, char ** argv) {
    // Lock all current and future pages from preventing of being paged to swap
    const int lockall_res = mlockall( MCL_CURRENT | MCL_FUTURE );
    if (lockall_res) { 
        fprintf(stderr, "mlockall failed: %d", lockall_res);
    }

    int ret = 0;

    dev_out_settings_t out_settings = {
        .default_gamepad = 0,
        .nintendo_layout = false,
        .gamepad_leds_control = true,
        .gamepad_rumble_control = true,
        .controller_bluetooth = false,
        .dualsense_edge = false,
        .swap_y_z = false,
        .invert_x = false,
        .gyro_to_analog_activation_treshold = 16,
        .gyro_to_analog_mapping = 4,
    };

    load_out_config(&out_settings, configuration_file);

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
        .settings = out_settings,
    };

    load_out_config(&dev_out_thread_data.settings, configuration_file);

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

    pthread_t dev_out_thread;
    const int dev_out_thread_creation = pthread_create(&dev_out_thread, &attr, dev_out_thread_func, (void*)(&dev_out_thread_data));
    if (dev_out_thread_creation != 0) {
        fprintf(stderr, "Error creating dev_out thread: %d\n", dev_out_thread_creation);
        ret = -1;
        //logic_request_termination(&global_logic);
        goto main_err;
    }

    const uint64_t timeout_ms = 1500;

    struct pollfd poll_fds[2];
    poll_fds[0].fd = sfd;
    poll_fds[0].events = POLL_IN;

    int    sd=-1;
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
        strncpy(serveraddr.sun_path, SERVER_PATH, sizeof(serveraddr.sun_path) - 1);

        int rc = bind(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
        if (rc < 0)
        {
            perror("bind() failed");
            break;
        }

        rc = listen(sd, MAX_CONNECTED_CLIENTS - 1);
        if (rc< 0)
        {
            perror("listen() failed");
            break;
        }

        poll_fds[1].fd = sd;
        poll_fds[1].events = POLL_IN;

        while (true) {
            poll_fds[0].revents = 0;
            poll_fds[1].revents = 0;

            const int poll_ret = poll(poll_fds, sizeof(poll_fds) / sizeof(poll_fds[0]), timeout_ms);

            if (poll_fds[0].revents & POLLIN) {
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
                    goto main_exit;
                } else if (si.ssi_signo == SIGINT) {
                    printf("Received SIGINT -- propagating signal\n");
                    goto main_exit;
                }
            } else if (poll_fds[1].revents & POLLIN) {
                const int client_fd = accept(sd, NULL, NULL);
                if (client_fd < 0) {
                    fprintf(stderr, "Error in getting a client connected: %d\n", client_fd);
                    continue;
                }

                // here the client_fd is good
                if (pthread_mutex_lock(&dev_out_thread_data.communication.endpoint.ssocket.mutex) == 0) {
                    bool found = false;
                    for (size_t i = 0; i < MAX_CONNECTED_CLIENTS; ++i) {
                        if (dev_out_thread_data.communication.endpoint.ssocket.clients[i] < 0) {
                            printf("Accepted new incoming connection on slot %zu: %d\n", i, client_fd);
                            dev_out_thread_data.communication.endpoint.ssocket.clients[i] = client_fd;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        fprintf(stderr, "Could not find a free spot fot the incoming client -- client will be rejected\n");
                        close(client_fd);
                    }

                    pthread_mutex_unlock(&dev_out_thread_data.communication.endpoint.ssocket.mutex);
                }
            }
        }
    } while (false);

main_exit:
    dev_out_thread_data.flags |= DEV_OUT_FLAG_EXIT;

    if (sd != -1) {
        close(sd);
    }

    unlink(SERVER_PATH);

main_err:
    if (dev_out_thread_creation == 0) {
        pthread_join(dev_out_thread, NULL);
        printf("dev_out_thread terminated\n");
    }

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
