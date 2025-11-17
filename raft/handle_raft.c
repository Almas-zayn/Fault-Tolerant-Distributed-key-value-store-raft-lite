#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "raft_node.h"
#include "cmd_queue.h"
#include "../rpc/rpc.h"
#include "../wal/wal.h"
#include "raft_helpers.h"
#include "../ui/colors.h"

void handle_raft_polls(int fd)
{
    int in_fd[NODES];
    memset(in_fd, -1, sizeof(in_fd));

    struct pollfd pf[1 + NODES];
    pf[0].fd = fd;
    pf[0].events = POLLIN;

    for (int i = 0; i < NODES; i++)
    {
        pf[1 + i].fd = -1;
        pf[1 + i].events = POLLIN;
    }

    long last_hb = now_ms();
    long last_leader_hb = now_ms();
    int election_timeout = 500 + (rand() % 500);

    usleep((rand() % 50) * 1000);

    while (1)
    {
        long ms = now_ms();

        if (raft_node.role != LEADER)
        {
            if (ms - last_hb >= election_timeout)
            {
                start_election(&last_leader_hb, &last_hb, &election_timeout, ms);
            }
        }

        if (raft_node.role == LEADER)
        {
            if (ms - last_leader_hb > 120)
            {
                leader_send_heartbeat_once();
                last_leader_hb = ms;
            }
        }

        int ret = poll(pf, 1 + NODES, 50);
        if (ret < 0)
        {
            perror("poll");
            exit(1);
        }
        if (pf[0].revents & POLLIN)
        {
            int c = accept(fd, NULL, NULL);
            for (int i = 0; i < NODES; i++)
            {
                if (in_fd[i] == -1)
                {
                    in_fd[i] = c;
                    pf[1 + i].fd = c;
                    break;
                }
            }
        }

        for (int i = 0; i < NODES; i++)
        {
            int idx = 1 + i;
            if (pf[idx].fd != -1 && (pf[idx].revents & POLLIN))
            {
                Raft_Req req;
                int r = read(pf[idx].fd, &req, sizeof(req));

                if (r <= 0)
                {
                    close(pf[idx].fd);
                    pf[idx].fd = -1;
                    in_fd[i] = -1;
                    continue;
                }

                if (req.term > raft_node.term)
                {
                    become_follower(req.term, req.id);
                    last_hb = ms;
                }

                if (req.req_type == REQUEST_VOTE)
                {
                    Raft_Res res;
                    memset(&res, 0, sizeof(res));
                    res.res_type = VOTE_DENIED;
                    res.term = raft_node.term;
                    res.id = raft_node.id;

                    int li, lt;
                    get_last_log(&li, &lt);

                    int ok = (req.last_log_term > lt ||
                              (req.last_log_term == lt && req.last_log_index >= li));

                    if (req.term == raft_node.term &&
                        ok &&
                        (raft_node.votedFor == -1 || raft_node.votedFor == req.id))
                    {
                        raft_node.votedFor = req.id;
                        persist_state();
                        res.res_type = GRANT_VOTE;
                        vprint_success("Node %d: voted for %d\n", raft_node.id, req.id);
                        last_hb = ms;
                    }
                    write(pf[idx].fd, &res, sizeof(res));
                }
                else if (req.req_type == APPEND_ENTRIES)
                {

                    Raft_Res res;
                    memset(&res, 0, sizeof(res));
                    res.term = raft_node.term;
                    res.id = raft_node.id;
                    res.res_type = APPEND_RESPONSE;
                    // printf("Node %d: Received APPEND_ENTRIES from %d term=%d prev_idx=%d prev_term=%d entries=%d leader_commit=%d\n",
                    //        raft_node.id, req.id, req.term, req.prev_log_index, req.prev_log_term, req.entries_count, req.leader_commit);

                    if (req.term < raft_node.term)
                    {
                        res.success = 0;
                        write(pf[idx].fd, &res, sizeof(res));
                        //   printf("Node %d: APPEND_ENTRIES rejected (term too old). my_term=%d req.term=%d\n", raft_node.id, raft_node.term, req.term);
                        continue;
                    }

                    raft_node.leader_id = req.id;
                    last_hb = ms;

                    if (req.term > raft_node.term || raft_node.role == CANDIDATE)
                        become_follower(req.term, req.id);

                    int last = raft_node.log_count - 1;
                    if (req.prev_log_index > last ||
                        (req.prev_log_index >= 0 && raft_node.log[req.prev_log_index].term != req.prev_log_term))
                    {
                        res.success = 0;
                        res.match_index = raft_node.log_count - 1;
                        write(pf[idx].fd, &res, sizeof(res));
                        vprint_error("Node %d: APPEND_ENTRIES mismatch prev_idx=%d last=%d -> reject\n", raft_node.id, req.prev_log_index, last);
                        continue;
                    }

                    int pos = req.prev_log_index + 1;
                    for (int j = 0; j < req.entries_count; j++)
                    {
                        LogEntry *le = &req.entries[j];

                        pthread_mutex_lock(&raft_lock);

                        if (pos < raft_node.log_count && raft_node.log[pos].term != le->term)
                        {
                            vprint_info("Node %d: conflict at pos=%d (existing_term=%d, incoming_term=%d) -> truncating\n",
                                        raft_node.id, pos, raft_node.log[pos].term, le->term);

                            wal_truncate_from(node_wal_fd, pos);
                            raft_node.log_count = pos;
                        }

                        if (pos == raft_node.log_count)
                        {
                            int wal_idx = append_log_entry_and_persist(le);
                            if (wal_idx != pos)
                            {
                                fprintf(stderr, "Node %d: ERROR: wal returned idx=%d expected=%d — aborting to avoid divergence\n",
                                        raft_node.id, wal_idx, pos);
                                pthread_mutex_unlock(&raft_lock);
                                exit(1);
                            }
                            raft_node.log[pos] = *le;
                            raft_node.log_count++;
                            pthread_mutex_unlock(&raft_lock);
                        }
                        else
                        {
                            raft_node.log[pos] = *le;
                            pthread_mutex_unlock(&raft_lock);
                        }

                        pos++;
                    }

                    if (req.leader_commit > raft_node.commitIndex)
                    {
                        int new_commit = req.leader_commit;
                        if (new_commit > raft_node.log_count - 1)
                            new_commit = raft_node.log_count - 1;
                        if (new_commit != raft_node.commitIndex)
                        {
                            raft_node.commitIndex = new_commit;
                            persist_state();
                        }
                    }

                    update_commit();

                    res.success = 1;
                    res.match_index = raft_node.log_count - 1;
                    write(pf[idx].fd, &res, sizeof(res));
                    //  printf("Node %d: APPEND_ENTRIES applied up to index=%d, commitIndex=%d lastApplied=%d\n",
                    //         raft_node.id, raft_node.log_count - 1, raft_node.commitIndex, raft_node.lastApplied);
                }
            }
        }

        if (raft_node.role == LEADER)
        {
            if (command_queue.count > 0)
            {
                LogEntry e;
                queue_pop(&command_queue, &e);

                pthread_mutex_lock(&raft_lock);
                e.term = raft_node.term;

                int idx = append_log_entry_and_persist(&e);
                raft_node.log[idx] = e;
                raft_node.log_count++;
                pthread_mutex_unlock(&raft_lock);

                vprint_success("Node %d: leader appended log index %d\n", raft_node.id, idx);

                leader_replicate_entry(idx);
            }
        }
    }
}