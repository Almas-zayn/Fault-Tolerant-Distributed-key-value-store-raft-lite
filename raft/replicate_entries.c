#include <string.h>
#include <unistd.h>
#include "raft_node.h"
#include "raft_helpers.h"

void leader_replicate_entry(int idx_of_entry)
{
    for (int i = 0; i < NODES; i++)
    {
        int peer = i + 1;
        if (peer == raft_node.id)
            continue;

        int attempts = 3;
        while (attempts--)
        {
            Raft_Req req;
            memset(&req, 0, sizeof(req));
            req.req_type = APPEND_ENTRIES;
            req.term = raft_node.term;
            req.id = raft_node.id;
            req.leader_commit = raft_node.commitIndex;

            int next = raft_node.nextIndex[i];
            if (next - 1 >= 0 && next - 1 < raft_node.log_count)
                req.prev_log_index = next - 1;
            else
                req.prev_log_index = raft_node.log_count - 1;

            if (req.prev_log_index >= 0 && req.prev_log_index < raft_node.log_count)
                req.prev_log_term = raft_node.log[req.prev_log_index].term;
            else
                req.prev_log_term = 0;

            int last = raft_node.log_count - 1;
            if (next <= last)
            {
                req.entries_count = 1;
                req.entries[0] = raft_node.log[next];
            }

            Raft_Res res;
            if (!send_append_entries_to_peer(peer, &req, &res))
            {
                usleep(20000);
                continue;
            }

            if (res.success)
            {
                raft_node.matchIndex[i] = res.match_index;
                raft_node.nextIndex[i] = res.match_index + 1;
                update_commit();
                break;
            }
            else
            {
                if (raft_node.nextIndex[i] > 1)
                    raft_node.nextIndex[i]--;
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
    int tries = 2;
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
            usleep(20000);
            continue;
        }

        int w = write(s, req, sizeof(*req));
        if (w != sizeof(*req))
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
        req.prev_log_index = raft_node.nextIndex[i] - 1;
        if (req.prev_log_index >= 0 && req.prev_log_index < raft_node.log_count)
            req.prev_log_term = raft_node.log[req.prev_log_index].term;
        else
            req.prev_log_term = 0;
        req.entries_count = 0;

        Raft_Res res;
        if (send_append_entries_to_peer(peer, &req, &res))
        {
            if (res.success)
            {
                raft_node.matchIndex[i] = res.match_index;
                raft_node.nextIndex[i] = res.match_index + 1;
                update_commit();
            }
            else
            {
                if (raft_node.nextIndex[i] > 1)
                    raft_node.nextIndex[i]--;
            }
        }
    }
}
