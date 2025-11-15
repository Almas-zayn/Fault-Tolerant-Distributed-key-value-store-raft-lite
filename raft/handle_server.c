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

void apply_log_entry(const LogEntry *e)
{
    if (e->command.req_type == PUT)
        kv_put_local(e->command.key, e->command.value);
    else if (e->command.req_type == DEL)
        kv_del_local(e->command.key);

    pthread_mutex_lock(&commit_lock);
    if (expected_commit_index == raft_node.lastApplied)
        pthread_cond_signal(&commit_cv);
    pthread_mutex_unlock(&commit_lock);
}

void handle_server_polls(int server_fd)
{
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i] = -1;

    struct pollfd pf[1 + MAX_CLIENTS];
    pf[0].fd = server_fd;
    pf[0].events = POLLIN;

    for (int i = 1; i <= MAX_CLIENTS; i++)
    {
        pf[i].fd = -1;
        pf[i].events = POLLIN;
    }

    while (1)
    {
        int r = poll(pf, 1 + MAX_CLIENTS, 100);
        if (r < 0)
            continue;
        // accepting clients
        if (pf[0].revents & POLLIN)
        {
            int c = accept(server_fd, NULL, NULL);
            if (c >= 0)
            {
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i] == -1)
                    {
                        clients[i] = c;
                        pf[1 + i].fd = c;
                        fcntl(c, F_SETFL, O_NONBLOCK);
                        printf("Server: accepted client\n");
                        break;
                    }
                }
            }
        }
    }
}
