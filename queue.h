#pragma once

#include "rogue_enemy.h"

typedef struct queue {
    sem_t empty, full;
    pthread_mutex_t mutex;

    ssize_t front;
    ssize_t rear;

    size_t array_size;
    void** array;
} queue_t;

int queue_init(queue_t* queue, size_t max_elements);

void queue_destroy(queue_t* queue);

int queue_push(queue_t* queue, void *in_item);

int queue_push_timeout(queue_t* const  q, void *in_item, int timeout_ms);

int queue_pop(queue_t* queue, void **out_item);

int queue_pop_timeout(queue_t* const q, void **out_item, int timeout_ms);