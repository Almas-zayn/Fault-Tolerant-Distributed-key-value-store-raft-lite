#ifndef _RAFT_NODE_
#define _RAFT_NODE_

#include "../rpc/rpc.h"

#define MAX_CLIENTS 10
#define NODES 5
#define IP_ADDR "127.0.0.1"
#define LOG_CAPACITY 1024

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
    int term;
    int votedFor;
} PersistentState;

typedef struct
{
    int id;
    int port;
    char wal_name[16];
    NodeRole role;
    int votesReceived;

    int term;
    int votedFor;
    LogEntry log[LOG_CAPACITY];
    int log_count;

} RaftNode;

typedef struct
{
    char key[32];
    char val[128];
    int used;
} kv_t;

extern RaftNode raft_node;
extern int node_wal_fd;

void persist_state();
void load_persistent_state();

void handle_raft_polls(int raft_fd);
void handle_server_polls(int server_fd);

#endif