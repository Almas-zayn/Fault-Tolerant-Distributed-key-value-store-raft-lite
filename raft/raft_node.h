#ifndef _RAFT_NODE_
#define _RAFT_NODE_

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

typedef enum
{
    REQUEST_VOTE,
    GRANT_VOTE,
    VOTE_DENIED,
    HEARTBEAT,
} peer_req_t;

void init_raft_node(int id, int port, char *wal_name);
int start_election();
int send_heartbeat();

#endif