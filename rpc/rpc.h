#ifndef RPC_H
#define RPC_H

#define MAX_KEY_LEN 32
#define MAX_VAL_LEN 128
#define MAX_LOG_ENTRIES_PER_RPC 32

typedef enum
{
    REQUEST_VOTE = 1,
    GRANT_VOTE,
    VOTE_DENIED,
    APPEND_ENTRIES,
    APPEND_RESPONSE
} raft_req_type;

typedef enum
{
    PUT = 1,
    GET,
    DEL
} kv_req_type;

typedef struct
{
    kv_req_type req_type;
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
} kv_req;

typedef struct
{
    int term;
    kv_req command;
} LogEntry;

typedef struct
{
    int status;
    int leader_id;
    char value[MAX_VAL_LEN];
} kv_res;

typedef struct
{
    raft_req_type req_type;
    int term;
    int id;

    int last_log_index;
    int last_log_term;

    int prev_log_index;
    int prev_log_term;

    int entries_count;
    LogEntry entries[MAX_LOG_ENTRIES_PER_RPC];

    int leader_commit;

} Raft_Req;

typedef struct
{
    raft_req_type res_type;
    int term;
    int id;
    int success;
    int match_index;
} Raft_Res;

int start_server(int port);
int connect_peer(const char *ip, int port);

#endif
