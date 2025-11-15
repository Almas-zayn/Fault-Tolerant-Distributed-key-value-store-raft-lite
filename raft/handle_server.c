#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>

#include "raft_node.h"
#include "cmd_queue.h"
#include "../rpc/rpc.h"

typedef struct
{
    char key[32];
    char val[128];
    int used;
} kv_t;

kv_t kv_store[1024];

pthread_mutex_t commit_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t commit_cv = PTHREAD_COND_INITIALIZER;
int expected_commit_index = -1;

int kv_put_local(const char *k, const char *v)
{
    for (int i = 0; i < 1024; i++)
    {
        if (kv_store[i].used && strcmp(kv_store[i].key, k) == 0)
        {
            strncpy(kv_store[i].val, v, sizeof(kv_store[i].val) - 1);
            return 1;
        }
    }
    for (int i = 0; i < 1024; i++)
    {
        if (!kv_store[i].used)
        {
            kv_store[i].used = 1;
            strncpy(kv_store[i].key, k, sizeof(kv_store[i].key) - 1);
            strncpy(kv_store[i].val, v, sizeof(kv_store[i].val) - 1);
            return 1;
        }
    }
    return 0;
}

int kv_get_local(const char *k, char *out)
{
    for (int i = 0; i < 1024; i++)
    {
        if (kv_store[i].used && strcmp(kv_store[i].key, k) == 0)
        {
            strncpy(out, kv_store[i].val, sizeof(kv_store[i].val) - 1);
            return 1;
        }
    }
    return 0;
}

int kv_del_local(const char *k)
{
    for (int i = 0; i < 1024; i++)
    {
        if (kv_store[i].used && strcmp(kv_store[i].key, k) == 0)
        {
            kv_store[i].used = 0;
            return 1;
        }
    }
    return 0;
}