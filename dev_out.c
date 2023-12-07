#include "dev_out.h"

#include "virt_ds4.h"

static void handle_incoming_message_gamepad_action(
    in_message_gamepad_action_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    
}

static void handle_incoming_message_gamepad_set(
    in_message_gamepad_set_element_t *const msg_payload,
    gamepad_status_t *const inout_gamepad
) {
    
}

static void handle_incoming_message(
    in_message_t *const msg,
    devices_status_t *const dev_stats
) {
    if (msg->type == GAMEPAD_SET_ELEMENT) {
        handle_incoming_message_gamepad_set(&msg->data.gamepad_set, dev_stats);
    } else if (msg->type == GAMEPAD_ACTION) {
        handle_incoming_message_gamepad_action(&msg->data.action, dev_stats);
    }
}

uint64_t get_timediff_usec(struct timeval past, struct timeval now) {
    return 0;
}

void *dev_out_thread_func(void *ptr) {
    dev_out_data_t *const dev_out = (dev_out_data_t*)ptr;

    // Initialize device
    devices_status_init(&dev_out->dev_stats);

    dev_out_gamepad_device_t current_gamepad = dev_out->gamepad;

    union {
        virt_dualshock_t ds4;
    } controller_data;

    int current_gamepad_fd = -1;
    int current_keyboard_fd = -1;
    int current_mouse_fd = -1;

    if (current_gamepad == GAMEPAD_DUALSENSE) {

    } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
        const int ds4_init_res = virt_dualshock_init(&controller_data.ds4);
        if (ds4_init_res != 0) {
            fprintf(stderr, "Unable to initialize the DualShock device: %d\n", ds4_init_res);
        } else {
            current_gamepad_fd = virt_dualshock_get_fd(&controller_data.ds4);
        }
    }

    // TODO: stats->gamepad.flags |= GAMEPAD_STATUS_FLAGS_PRESS_AND_REALEASE_CENTER;

    struct timeval now = {0};
    gettimeofday(&now, NULL);

    struct timeval gamepad_last_hid_report_sent = now;
    struct timeval mouse_last_hid_report_sent = now;
    struct timeval keyboard_last_hid_report_sent = now;

    fd_set read_fds;
    for (;;) {
        FD_ZERO(&read_fds);        

        if (current_gamepad == GAMEPAD_DUALSENSE) {

        } else if (current_gamepad == GAMEPAD_DUALSHOCK) {
            
        }

        FD_SET(dev_out->out_message_pipe_fd, &read_fds);
        // TODO: FD_SET(current_mouse_fd, &read_fds);
        // TODO: FD_SET(current_keyboard_fd, &read_fds);
        FD_SET(current_gamepad_fd, &read_fds);

        // TODO: calculate the shortest time between multiple device
        struct timeval timeout = {
            //.tv_sec = (__time_t)devs->timeout_ms / (__time_t)1000,
            //.tv_usec = ((__suseconds_t)devs->timeout_ms % (__suseconds_t)1000) * (__suseconds_t)1000000,
        };

        gettimeofday(&now, NULL);

        int ready_fds = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);

        if (ready_fds == -1) {
            const int err = errno;
            fprintf(stderr, "Error reading devices: %d\n", err);
            continue;
        } else if (ready_fds == 0) {
            // Timeout... simply retry
            continue;
        }

        if (FD_ISSET(dev_out->in_message_pipe_fd, &read_fds)) {
            in_message_t incoming_message;
            size_t in_message_pipe_read_res = read(dev_out->in_message_pipe_fd, (void*)&incoming_message, sizeof(in_message_t));
            if (in_message_pipe_read_res == sizeof(in_message_t)) {
                handle_incoming_message(&incoming_message, &dev_out->dev_stats);
            } else {
                fprintf(stderr, "Error reading from out_message_pipe_fd: got %zu bytes, expected %zu butes", in_message_pipe_read_res, sizeof(in_message_t));
            }
        
        }
    }

    return NULL;
}
