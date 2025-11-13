#ifndef RPC_H
#define RPC_H

#include "../raft/raft_node.h"

typedef enum
{
    REQUEST_VOTE,
    GRANT_VOTE,
    VOTE_DENIED,
    HEARTBEAT,
} raft_req_t;

typedef enum
{
    PUT,
    GET,
    DEL,
} kv_req_t;

typedef struct
{
    char key[120];
    char value[120];
} kv_req;

typedef struct
{
    int status;
    char msg[120];
} kv_res;

typedef struct
{
    raft_req_t req_type;
    int term;
    int id;
} raft_req;

typedef struct
{
    raft_req_t res_type;
    int term;
    int id;
} raft_res;

int start_server(int port);
int send_raft_rpc_req(raft_req req);
int send_raft_rpc_res(raft_res res);
int send_kv_rpc_res(kv_req req);
int connect_and_send(int port, void *buf, size_t len);

#endif