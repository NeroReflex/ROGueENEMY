#pragma once

#include "rogue_enemy.h"

typedef struct ev_queue {
    int sem_id;

    size_t in;
    size_t out;

    size_t array_size;
    void** array;
} ev_queue_t;

int queue_init(ev_queue_t* queue, key_t sem_key, size_t max_elements);

void queue_destroy(ev_queue_t* queue);

int queue_push(ev_queue_t* queue, void *in_item);

int queue_pop(ev_queue_t* queue, void **out_item);