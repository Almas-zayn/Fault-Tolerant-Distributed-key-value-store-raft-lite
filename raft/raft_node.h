#ifndef _RAFT_NODE_
#define _RAFT_NODE_

#define MAX_CLIENTS 10
#define NODES 5
#define IP_ADDR "127.0.0.1"

typedef struct NodeConfig
{
    int id;
    int port;
    char wal_name[16];
} nodeConfig_t;

typedef enum
{
    FOLLOWER,
    CANDIDATE,
    LEADER
} NodeRole;

typedef struct
{
    int id;
    int port;
    int term;
    char wal_name[16];
    int votedFor;
    NodeRole role;
    int votesReceived;
} RaftNode;

void init_raft_node(int id, int port, char *wal_name);
void start_election(RaftNode *node);
int send_heartbeat();

#endif