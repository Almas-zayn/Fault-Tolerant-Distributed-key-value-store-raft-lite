#ifndef _RAFT_NODE_H_
#define _RAFT_NODE_H_

#include "../rpc/rpc.h"
#include "cmd_queue.h"

#define NODES 5
#define IP_ADDR "127.0.0.1"
#define LOG_CAPACITY 4096
#define MAX_CLIENTS 10

typedef enum
{
    FOLLOWER = 1,
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
    char wal_name[32];
    NodeRole role;

    int term;
    int votedFor;
    int votesReceived;

    LogEntry log[LOG_CAPACITY];
    int log_count;

    int commitIndex;
    int lastApplied;

    int nextIndex[NODES];
    int matchIndex[NODES];

    int leader_id;

} RaftNode;

extern RaftNode raft_node;
extern int node_wal_fd;
extern struct CmdQueue command_queue;

void persist_state();
void load_persistent_state();
int append_log_entry_and_persist(const LogEntry *e);

void handle_raft_polls(int fd);
void handle_server_polls(int fd);

void apply_log_entry(const LogEntry *e);

#endif