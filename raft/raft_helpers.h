#ifndef _RAFT_HELPERS_
#define _RAFT_HELPERS_

#include "../rpc/rpc.h"

void init_leader_state();
void become_follower(int t, int leader);

void get_last_log(int *idx, int *term);
void update_commit();
long now_ms();

void leader_replicate_entry(int idx_of_entry);
void leader_send_heartbeat_once();

int send_append_entries_to_peer(int peer_id, Raft_Req *req, Raft_Res *out_res);
int send_request_vote_to_peer(int peer_id);

void start_election(long *last_leader_hb, long *last_hb, int *election_timeout, long ms);

#endif