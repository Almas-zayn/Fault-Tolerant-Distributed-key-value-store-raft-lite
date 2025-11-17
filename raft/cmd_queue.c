#include <string.h>
#include <stdio.h>
#include "cmd_queue.h"

void queue_init(CmdQueue *q)
{
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond_nonempty, NULL);
}

void queue_push(CmdQueue *q, LogEntry *e)
{
    pthread_mutex_lock(&q->lock);

    q->entries[q->tail] = *e;
    q->tail = (q->tail + 1) % 128;
    q->count++;

    pthread_cond_signal(&q->cond_nonempty);
    pthread_mutex_unlock(&q->lock);
}

int queue_pop(CmdQueue *q, LogEntry *out)
{
    pthread_mutex_lock(&q->lock);

    while (q->count == 0)
    {
        printf("waiting in queue\n");
        pthread_cond_wait(&q->cond_nonempty, &q->lock);
    }

    *out = q->entries[q->head];
    q->head = (q->head + 1) % 128;
    q->count--;

    pthread_mutex_unlock(&q->lock);
    return 1;
}
