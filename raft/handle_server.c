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
#include "../ui/colors.h"

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
                        vprint_success("Server: accepted client\n");
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int idx = 1 + i;
            if (pf[idx].fd != -1 && (pf[idx].revents & POLLIN))
            {
                kv_req req;
                int n = read(pf[idx].fd, &req, sizeof(req));
                if (n <= 0)
                {
                    close(pf[idx].fd);
                    pf[idx].fd = -1;
                    clients[i] = -1;
                    continue;
                }

                kv_res res;
                memset(&res, 0, sizeof(res));

                if (req.req_type == GET)
                {
                    char buf[MAX_VAL_LEN];
                    if (kv_get_local(req.key, buf))
                    {
                        strncpy(res.value, buf, MAX_VAL_LEN);
                        res.status = 1;
                        vprint_success("GET key=%s found=%s\n", req.key, buf);
                    }
                    else
                    {
                        res.status = 0;
                        vprint_info("GET key=%s not_found\n", req.key);
                    }

                    write(pf[idx].fd, &res, sizeof(res));
                    continue;
                }

                if (raft_node.role != LEADER)
                {
                    res.status = 0;
                    res.leader_id = raft_node.leader_id;
                    vprint_info("PUT/DEL forwarded to leader %d\n", raft_node.leader_id);
                    write(pf[idx].fd, &res, sizeof(res));
                    continue;
                }

                LogEntry e;
                memset(&e, 0, sizeof(e));
                e.term = raft_node.term;
                e.command = req;

                int new_index = raft_node.log_count;
                expected_commit_index = new_index;

                queue_push(&command_queue, &e);
                vprint_info("Leader received client req, queued index=%d\n", new_index);

                pthread_mutex_lock(&commit_lock);
                while (raft_node.lastApplied < expected_commit_index)
                {
                    pthread_cond_wait(&commit_cv, &commit_lock);
                }
                pthread_mutex_unlock(&commit_lock);

                res.status = 1;
                vprint_info("PUT/DEL committed index=%d\n", expected_commit_index);

                write(pf[idx].fd, &res, sizeof(res));
            }
        }
    }
}
