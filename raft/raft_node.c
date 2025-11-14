#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "raft_node.h"
#include "../rpc/rpc.h"
#include "../wal/wal.h"

RaftNode raft_node;
CmdQueue command_queue;
int node_wal_fd;

void persist_state()
{
    lseek(node_wal_fd, 0, SEEK_SET);
    PersistentState ps;
    ps.term = raft_node.term;
    ps.votedFor = raft_node.votedFor;
    write(node_wal_fd, &ps, sizeof(ps));
    fsync(node_wal_fd);
}

void load_persistent_state()
{
    lseek(node_wal_fd, 0, SEEK_SET);
    PersistentState ps;
    read(node_wal_fd, &ps, sizeof(ps));

    raft_node.term = ps.term;
    raft_node.votedFor = ps.votedFor;

    printf("Loaded persistent state: term=%d votedFor=%d\n", ps.term, ps.votedFor);
}

int append_log_entry_and_persist(const LogEntry *e)
{
    int idx = -1;
    idx = wal_append_entry(node_wal_fd, e, &idx);
    return idx;
}

void *raft_thread_func(void *arg)
{
    int raft_fd = start_server(raft_node.port);
    if (raft_fd < 0)
    {
        printf("Raft server start failed\n");
        exit(1);
    }

    printf("Raft Node ID: %d -> Raft server on %d\n", raft_node.id, raft_node.port);
    handle_raft_polls(raft_fd);
    return NULL;
}

void *server_thread_func(void *arg)
{
    int server_port = raft_node.port + 1000;
    int sfd = start_server(server_port);
    if (sfd < 0)
    {
        printf("Client server start failed\n");
        exit(1);
    }

    printf("Client Server started on %d\n", server_port);
    handle_server_polls(sfd);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <id> <port> <walfile>\n", argv[0]);
        return 1;
    }
    srand(getpid());
    raft_node.id = atoi(argv[1]);
    raft_node.port = atoi(argv[2]);
    strncpy(raft_node.wal_name, argv[3], sizeof(raft_node.wal_name) - 1);

    raft_node.role = FOLLOWER;
    raft_node.votedFor = -1;
    raft_node.leader_id = -1;
    raft_node.log_count = 0;
    raft_node.commitIndex = -1;
    raft_node.lastApplied = -1;

    for (int i = 0; i < LOG_CAPACITY; i++)
        memset(&raft_node.log[i], 0, sizeof(LogEntry));

    node_wal_fd = wal_init(raft_node.wal_name);
    if (node_wal_fd < 0)
    {
        printf("wal init failed\n");
        return 1;
    }

    load_persistent_state();
    wal_load_all();

    printf("raft started: ID=%d Port=%d WAL=%s\n",
           raft_node.id, raft_node.port, raft_node.wal_name);

    pthread_t raft_thread, server_thread;

    pthread_create(&raft_thread, NULL, raft_thread_func, NULL);
    pthread_create(&server_thread, NULL, server_thread_func, NULL);

    pthread_join(raft_thread, NULL);
    pthread_join(server_thread, NULL);

    return 0;
}