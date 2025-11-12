#include <stdio.h>
#include <string.h>
#include "raft_node.h"

RaftNode node;
int node_wal;

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <node_id> <port> <wal_name>\n", argv[0]);
        return 1;
    }
    init_raft_node(atoi(argv[1]), atoi(argv[2]), argv[3]);
}

void init_raft_node(int id, int port, char *wal_name)
{
    node.id = id;
    strcpy(node.wal_name, wal_name);
    node.role = FOLLOWER;
    node.port = port;
    node.votedFor = -1;
    node.term = 0;
    node.votesReceived = 0;
}