#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "raft_node.h"
#include "raft_helpers.h"
#include "../ui/colors.h"
#include "../rpc/rpc.h"

void leader_replicate_entry(int idx_of_entry)
{
    for (int i = 0; i < NODES; i++)
    {
        int peer = i + 1;
        if (peer == raft_node.id)
            continue;

        int attempts = 10;
        while (attempts--)
        {
            Raft_Req req;
            memset(&req, 0, sizeof(req));
            req.req_type = APPEND_ENTRIES;
            req.term = raft_node.term;
            req.id = raft_node.id;
            req.leader_commit = raft_node.commitIndex;

            int next = raft_node.nextIndex[i];
            req.prev_log_index = next - 1;
            if (req.prev_log_index >= 0 && req.prev_log_index < raft_node.log_count)
                req.prev_log_term = raft_node.log[req.prev_log_index].term;
            else
                req.prev_log_term = 0;

            int last = raft_node.log_count - 1;
            if (next <= last)
            {
                int end = next + MAX_LOG_ENTRIES_PER_RPC - 1;
                if (end > last)
                    end = last;
                int cnt = end - next + 1;
                if (cnt < 0)
                    cnt = 0;

                req.entries_count = cnt;
                for (int j = 0; j < cnt; j++)
                    req.entries[j] = raft_node.log[next + j];
            }
            else
            {
                req.entries_count = 0;
            }

            vprint_info("Node %d: send AppendEntries to peer %d prev_idx = %d prev_term = %d entries = %d leader_commit = %d\n",
                        raft_node.id, peer, req.prev_log_index, req.prev_log_term, req.entries_count, req.leader_commit);

            Raft_Res res;
            if (!send_append_entries_to_peer(peer, &req, &res))
            {
                vprint_error("Node %d: failed to contact peer %d while replicating idx = %d\n", raft_node.id, peer, idx_of_entry);
                usleep(20000);
                continue;
            }

            if (res.success)
            {
                pthread_mutex_lock(&raft_lock);
                raft_node.matchIndex[i] = res.match_index;
                raft_node.nextIndex[i] = res.match_index + 1;
                pthread_mutex_unlock(&raft_lock);

                vprint_info("Node %d: peer %d acked match_index = %d\n", raft_node.id, peer, res.match_index);
                update_commit();
                break;
            }
            else
            {
                pthread_mutex_lock(&raft_lock);
                if (res.res_type == REQUEST_ENTRIES)
                {
                    int new_next = res.match_index + 1;
                    if (new_next < 0)
                        new_next = 0;
                    raft_node.nextIndex[i] = new_next;
                    vprint_info("Node %d: peer %d requested entries; set nextIndex to %d\n",
                                raft_node.id, peer, raft_node.nextIndex[i]);
                }
                else
                {
                    raft_node.nextIndex[i]--;
                    if (raft_node.nextIndex[i] < 0)
                        raft_node.nextIndex[i] = 0;
                    vprint_info("Node %d: peer %d did not match prev log; decrementing nextIndex to %d\n",
                                raft_node.id, peer, raft_node.nextIndex[i]);
                }
                pthread_mutex_unlock(&raft_lock);
            }
        }
    }
}

int send_request_vote_to_peer(int peer_id)
{
    Raft_Req rv;
    memset(&rv, 0, sizeof(rv));
    rv.req_type = REQUEST_VOTE;
    rv.term = raft_node.term;
    rv.id = raft_node.id;
    get_last_log(&rv.last_log_index, &rv.last_log_term);

    int peer_port = 5000 + peer_id;
    int tries = 1;
    while (tries--)
    {
        int s = connect_peer(IP_ADDR, peer_port);
        if (s < 0)
        {
            usleep(20000);
            continue;
        }

        int w = write(s, &rv, sizeof(rv));
        if (w != sizeof(rv))
        {
            close(s);
            return 0;
        }

        Raft_Res res;
        int r = read(s, &res, sizeof(res));
        close(s);
        if (r != sizeof(res))
            return 0;

        if (res.term > raft_node.term)
        {
            become_follower(res.term, -1);
            return 0;
        }

        if (res.res_type == GRANT_VOTE && res.term == raft_node.term)
            return 1;
        else
            return 0;
    }
    return 0;
}

int send_append_entries_to_peer(int peer_id, Raft_Req *req, Raft_Res *out_res)
{
    int peer_port = 5000 + peer_id;
    int tries = 2;
    while (tries--)
    {
        int s = connect_peer(IP_ADDR, peer_port);
        if (s < 0)
        {
            vprint_error("Node %d: connect_peer failed to %d:%d\n", raft_node.id, peer_id, peer_port);
            usleep(20000);
            continue;
        }

        int w = write(s, req, sizeof(*req));
        if (w != sizeof(*req))
        {
            vprint_error("Node %d: write to peer %d failed (w = %d)\n", raft_node.id, peer_id, w);
            close(s);
            return 0;
        }

        Raft_Res res;
        int r = read(s, &res, sizeof(res));
        close(s);
        if (r != sizeof(res))
        {
            vprint_error("Node %d: read from peer %d failed (r = %d)\n", raft_node.id, peer_id, r);
            return 0;
        }

        if (res.term > raft_node.term)
        {
            become_follower(res.term, -1);
            return 0;
        }

        if (out_res)
            *out_res = res;
        return 1;
    }
    return 0;
}

void leader_send_heartbeat_once()
{
    for (int i = 0; i < NODES; i++)
    {
        int peer = i + 1;
        if (peer == raft_node.id)
            continue;

        Raft_Req req;
        memset(&req, 0, sizeof(req));
        req.req_type = APPEND_ENTRIES;
        req.term = raft_node.term;
        req.id = raft_node.id;
        req.leader_commit = raft_node.commitIndex;

        int next = raft_node.nextIndex[i];
        req.prev_log_index = next - 1;
        if (req.prev_log_index >= 0 && req.prev_log_index < raft_node.log_count)
            req.prev_log_term = raft_node.log[req.prev_log_index].term;
        else
            req.prev_log_term = 0;

        int last = raft_node.log_count - 1;
        if (next <= last)
        {
            int end = next + MAX_LOG_ENTRIES_PER_RPC - 1;
            if (end > last)
                end = last;
            int cnt = end - next + 1;
            if (cnt < 0)
                cnt = 0;

            req.entries_count = cnt;
            for (int j = 0; j < cnt; j++)
                req.entries[j] = raft_node.log[next + j];
        }
        else
        {
            req.entries_count = 0;
        }

        Raft_Res res;
        if (send_append_entries_to_peer(peer, &req, &res))
        {
            if (res.success)
            {
                pthread_mutex_lock(&raft_lock);
                raft_node.matchIndex[i] = res.match_index;
                raft_node.nextIndex[i] = res.match_index + 1;
                pthread_mutex_unlock(&raft_lock);
                update_commit();
            }
            else
            {
                pthread_mutex_lock(&raft_lock);
                if (res.res_type == REQUEST_ENTRIES)
                {
                    int new_next = res.match_index + 1;
                    if (new_next < 0)
                        new_next = 0;
                    raft_node.nextIndex[i] = new_next;
                    vprint_info("Node %d: heartbeat -> peer %d requested entries; set nextIndex to %d\n",
                                raft_node.id, peer, raft_node.nextIndex[i]);
                }
                else
                {
                    raft_node.nextIndex[i]--;
                    if (raft_node.nextIndex[i] < 0)
                        raft_node.nextIndex[i] = 0;
                    vprint_info("Node %d: heartbeat -> peer %d mismatch; nextIndex now %d\n",
                                raft_node.id, peer, raft_node.nextIndex[i]);
                }
                pthread_mutex_unlock(&raft_lock);
            }
        }
    }
}