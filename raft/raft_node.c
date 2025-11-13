#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include "raft_node.h"
#include "../rpc/rpc.h"

RaftNode node;
int node_wal;

int handle_rafts_polls(struct pollfd *polls, nfds_t nfds)
{
    poll(polls, nfds, -1);
    if (polls[0].revents & POLLIN)
    {
        // accept rafts msg
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <node_id> <port> <wal_name>\n", argv[0]);
        return 1;
    }
    int wal_fd;
    init_raft_node(atoi(argv[1]), atoi(argv[2]), argv[3]);

    wal_fd = open(node.wal_name, O_CREAT | O_APPEND);
    int raft_fd = start_server(node.port);
    int server_fd = start_server(node.port + 1000);

    struct pollfd raft_poll[NODES + 1];
    raft_poll[0].fd = raft_fd;
    raft_poll[0].events = POLLIN;

    struct pollfd server_poll[MAX_CLIENTS + 1];
    server_poll[0].fd = server_fd;
    server_poll[0].events = POLLIN;
}

int handle_server_polls(struct pollfd *polls, nfds_t nfds)
{
}

void init_raft_node(int id, int port, char *wal_name)
{
    node.id = id;
    node.port = port;
    strncpy(node.wal_name, wal_name, sizeof(node.wal_name) - 1);
    node.role = FOLLOWER;
    node.votedFor = -1;
    node.term = 0;
    node.votesReceived = 0;
}