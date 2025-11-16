#include <time.h>
#include <stdio.h>
#include "raft_node.h"

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
            printf("Node %d: commitIndex advanced to %d\n", raft_node.id, raft_node.commitIndex);
            persist_state();
            while (raft_node.lastApplied < raft_node.commitIndex)
            {
                raft_node.lastApplied++;
                apply_log_entry(&raft_node.log[raft_node.lastApplied]);
                printf("Node %d: applied log index %d\n", raft_node.id, raft_node.lastApplied);
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
    printf("Node %d: became follower term=%d leader=%d\n", raft_node.id, t, leader);
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