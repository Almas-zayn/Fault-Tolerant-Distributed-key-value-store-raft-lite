#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H

#include <pthread.h>
#include "../rpc/rpc.h"
#include "raft_node.h"

typedef struct CmdQueue
{
    LogEntry entries[128];
    int head;
    int tail;
    int count;

    pthread_mutex_t lock;
    pthread_cond_t cond_nonempty;
} CmdQueue;

void queue_init(CmdQueue *q);
void queue_push(CmdQueue *q, LogEntry *e);
int queue_pop(CmdQueue *q, LogEntry *out);

#endif
