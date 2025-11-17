#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "raft_node.h"
#include "raft_helpers.h"
#include "../ui/colors.h"

long now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void get_last_log(int *idx, int *term)
{
    *idx = raft_node.log_count - 1;
    if (*idx < 0)
    {
        *idx = -1;
        *term = 0;
    }
    else
    {
        *term = raft_node.log[*idx].term;
    }
}

void start_election(long *last_leader_hb, long *last_hb, int *election_timeout, long ms)
{
    pthread_mutex_lock(&raft_lock);
    raft_node.role = CANDIDATE;
    raft_node.term++;
    raft_node.votedFor = raft_node.id;
    raft_node.votesReceived = 1;
    raft_node.leader_id = -1;
    persist_state();
    pthread_mutex_unlock(&raft_lock);

    vprint_info("Node %d: starting election for term = %d\n", raft_node.id, raft_node.term);

    int votes = 1;
    for (int i = 0; i < NODES; i++)
    {
        int peer = i + 1;
        if (peer == raft_node.id)
            continue;

        int got = send_request_vote_to_peer(peer);
        if (got)
        {
            votes++;
            vprint_info("Node %d: got vote from %d (total %d)\n", raft_node.id, peer, votes);
        }
    }

    raft_node.votesReceived = votes;
    if (raft_node.votesReceived > NODES / 2)
    {
        raft_node.role = LEADER;
        raft_node.leader_id = raft_node.id;
        init_leader_state();
        *last_leader_hb = ms;
        vprint_success("Node %d: became leader for term = %d\n", raft_node.id, raft_node.term);
    }

    *last_hb = ms;
    *election_timeout = 550 + (rand() % 500);
}

void update_commit()
{
    if (raft_node.role != LEADER)
        return;

    int last = raft_node.log_count - 1;
    for (int N = last; N > raft_node.commitIndex; N--)
    {
        if (raft_node.log[N].term != raft_node.term)
            continue;

        int count = 1;
        for (int i = 0; i < NODES; i++)
        {
            if (i == raft_node.id - 1)
                continue;
            if (raft_node.matchIndex[i] >= N)
                count++;
        }

        if (count > NODES / 2)
        {
            raft_node.commitIndex = N;
            vprint_info("Node %d: commitIndex advanced to %d\n", raft_node.id, raft_node.commitIndex);
            persist_state();
            while (raft_node.lastApplied < raft_node.commitIndex)
            {
                raft_node.lastApplied++;
                apply_log_entry(&raft_node.log[raft_node.lastApplied]);
                vprint_info("Node %d: applied log index %d\n", raft_node.id, raft_node.lastApplied);
            }

            break;
        }
    }
}

void become_follower(int t, int leader)
{
    pthread_mutex_lock(&raft_lock);
    raft_node.role = FOLLOWER;
    raft_node.term = t;
    raft_node.votedFor = -1;
    raft_node.leader_id = leader;
    persist_state();
    pthread_mutex_unlock(&raft_lock);
    vprint_info("Node %d: became follower for term = %d, voted to Node : %d\n", raft_node.id, t, leader);
}

void init_leader_state()
{
    int last = raft_node.log_count - 1;
    for (int i = 0; i < NODES; i++)
    {
        raft_node.nextIndex[i] = last + 1;
        raft_node.matchIndex[i] = 0;
    }
}