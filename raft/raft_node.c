#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "raft_node.h"
#include "../rpc/rpc.h"

RaftNode raft_node;
int node_wal_fd;

void *raft_thread(void *arg);
void *server_thread(void *arg);
void init_raft_node(int id, int port, char *wal_name);

void persist_state()
{
    if (node_wal_fd < 0)
        return;

    PersistentState state;
    state.term = raft_node.term;
    state.votedFor = raft_node.votedFor;

    lseek(node_wal_fd, 0, SEEK_SET);
    write(node_wal_fd, &state, sizeof(PersistentState));
    fsync(node_wal_fd);
}

void load_persistent_state()
{
    if (node_wal_fd < 0)
        return;

    PersistentState state;
    if (read(node_wal_fd, &state, sizeof(PersistentState)) == sizeof(PersistentState))
    {
        raft_node.term = state.term;
        raft_node.votedFor = state.votedFor;
        printf("Loaded persistent state: term=%d, votedFor=%d\n", raft_node.term, raft_node.votedFor);
    }
    else
    {
        printf("No persistent state found. Initializing.\n");
        persist_state();
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <node_id> <port> <wal_name>\n", argv[0]);
        return 1;
    }

    srand(getpid());
    char wal_path[64];
    snprintf(wal_path, sizeof(wal_path), "./%s", argv[3]);
    node_wal_fd = open(wal_path, O_RDWR | O_CREAT, 0666);
    if (node_wal_fd < 0)
    {
        perror("Error opening WAL file");
        return 1;
    }

    init_raft_node(atoi(argv[1]), atoi(argv[2]), argv[3]);
    printf("raft started : ID : %d, Port : %d, wal file name: %s\n", raft_node.id, raft_node.port, wal_path);

    int raft_fd = start_server(raft_node.port);
    int server_fd = start_server(raft_node.port + 1000);

    printf("Raft Node ID : %d -> Raft Server started on Port : %d\n", raft_node.id, raft_node.port);
    printf("Client Server started on Port : %d\n", raft_node.port + 1000);

    pthread_t raft_thread_id, client_thread_id;
    pthread_create(&raft_thread_id, NULL, raft_thread, &raft_fd);
    pthread_create(&client_thread_id, NULL, server_thread, &server_fd);

    pthread_join(raft_thread_id, NULL);
    pthread_join(client_thread_id, NULL);

    close(node_wal_fd);
    return 0;
}

void init_raft_node(int id, int port, char *wal_name)
{
    raft_node.id = id;
    raft_node.port = port;
    strncpy(raft_node.wal_name, wal_name, sizeof(raft_node.wal_name) - 1);
    raft_node.role = FOLLOWER;
    raft_node.votesReceived = 0;

    raft_node.log_count = 1;
    raft_node.log[0].term = 0;

    raft_node.term = 0;
    raft_node.votedFor = -1;
    load_persistent_state();
}

void *raft_thread(void *arg)
{
    int raft_listenerFD = *(int *)arg;
    handle_raft_polls(raft_listenerFD);
    return NULL;
}

void *server_thread(void *arg)
{
    int server_listenerFD = *(int *)arg;
    handle_server_polls(server_listenerFD);
    return NULL;
}