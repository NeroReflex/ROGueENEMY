#include "queue.h"
#include <stdlib.h>

int queue_init(ev_queue_t* queue, key_t sem_key, size_t max_elements) {
    queue->array_size = max_elements;
    queue->array = calloc(sizeof(void*), max_elements);
    if (queue->array == NULL) {
        perror("calloc");
        return -1;
    }

    int semid = semget(sem_key, 1, IPC_CREAT | 0666);
    if (semid < 0)
    {
        free(queue->array);
        perror("semget");
        return 1;
    }

    if (semctl(semid, 0, SETVAL, 1) < 0)
    {
        free(queue->array);
        perror("semctl");
        return 1;
    }

    return 0;
}

void queue_destroy(ev_queue_t* queue) {
    free(queue->array);
    queue->array = NULL;
}

int queue_push(ev_queue_t* queue, void *in_item) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(queue->sem_id, &sem_op, 1);

    queue->array[queue->in] = in_item;
    queue->in = (queue->in + 1) % queue->array_size;

    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    semop(queue->sem_id, &sem_op, 1);

    return 0;
}

int queue_pop(ev_queue_t* queue, void **out_item) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(queue->sem_id, &sem_op, 1);

    *out_item = queue->array[queue->out];
    queue->out = (queue->out + 1) % queue->array_size;

    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    semop(queue->sem_id, &sem_op, 1);
    
    return 0;
}
