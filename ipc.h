#pragma once

#include "rogue_enemy.h"

#define MAX_CONNECTED_CLIENTS 8

typedef struct ipc_strategy_socket {
    struct sockaddr_un serveraddr;
    int fd;
} ipc_strategy_socket_t;

typedef struct ipc_strategy_ssocket {
    pthread_mutex_t mutex;
    int clients[MAX_CONNECTED_CLIENTS];
} ipc_strategy_ssocket_t;

typedef struct ipc_strategy_pipe {

    // this pipe is reserved for reporting in_message_t
    int in_message_pipe_fd;

    // this messages is reserved for receiving out_message_t
    int out_message_pipe_fd;

} ipc_strategy_pipe_t;

typedef enum ipc_strategy {
    ipc_unix_pipe,
    ipc_server_sockets,
    ipc_client_socket,
} ipc_strategy_t;

typedef struct ipc {
    ipc_strategy_t type;
    union {
        ipc_strategy_pipe_t pipe;
        ipc_strategy_ssocket_t ssocket;
        ipc_strategy_socket_t socket;
    } endpoint;

} ipc_t;

#define SERVER_PATH "/tmp/rogue-enemy.sock"