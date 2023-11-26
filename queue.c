#include "queue.h"

int queue_init(queue_t* const q, size_t max_elements) {
    q->front = q->rear = -1;
    q->array_size = max_elements;
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("mutex");
        return -1;
    }

    q->array = calloc(sizeof(void*), max_elements);
    if (q->array == NULL) {
        perror("calloc");
        return -1;
    }

    sem_init(&q->empty, 0, q->array_size);
    sem_init(&q->full, 0, 0);

    return 0;
}

void queue_destroy(queue_t* q) {
    free(q->array);
    q->array = NULL;
}

int queue_push(queue_t* const  q, void *in_item) {
    sem_wait(&q->empty);
    pthread_mutex_lock(&q->mutex);

    q->rear = (q->rear + 1) % q->array_size;
    q->array[q->rear] = in_item;

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->full);

    return 0;
}

int queue_pop(queue_t* const q, void **out_item) {
    sem_wait(&q->full);
    pthread_mutex_lock(&q->mutex);

    q->front = (q->front + 1) % q->array_size;
    *out_item = q->array[q->front];

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->empty);
    
    return 0;
}

int queue_push_timeout(queue_t* const  q, void *in_item, int timeout_ms) {
    struct timespec timeout;
    if (clock_gettime(CLOCK_REALTIME, &timeout) == -1) {
        // Handle clock_gettime error
        return -1;
    }
    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000;

    int result = sem_timedwait(&q->empty, &timeout);

    if (result == 0) {
        pthread_mutex_lock(&q->mutex);

        q->rear = (q->rear + 1) % q->array_size;
        q->array[q->rear] = in_item;

        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->full);
    }

    return result;
}

int queue_pop_timeout(queue_t* const q, void **out_item, int timeout_ms) {
    struct timespec timeout;
    if (clock_gettime(CLOCK_MONOTONIC, &timeout) == -1) {
        // Handle clock_gettime error
        return -1;
    }
    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000;

    int result = sem_timedwait(&q->full, &timeout);

    if (result == 0) {
        pthread_mutex_lock(&q->mutex);

        q->front = (q->front + 1) % q->array_size;
        *out_item = q->array[q->front];

        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->empty);
    }

    return result;
}