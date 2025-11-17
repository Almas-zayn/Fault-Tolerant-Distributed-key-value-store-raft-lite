#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "raft_node.h"
#include "../rpc/rpc.h"
#include "../wal/wal.h"
#include "cmd_queue.h"
#include "../ui/colors.h"

RaftNode raft_node;
CmdQueue command_queue;
int node_wal_fd;

pthread_mutex_t raft_lock = PTHREAD_MUTEX_INITIALIZER;

void persist_state()
{
    lseek(node_wal_fd, 0, SEEK_SET);
    PersistentState ps;
    ps.term = raft_node.term;
    ps.votedFor = raft_node.votedFor;
    ps.commitIndex = raft_node.commitIndex;
    write(node_wal_fd, &ps, sizeof(ps));
    fsync(node_wal_fd);
}

void load_persistent_state()
{
    lseek(node_wal_fd, 0, SEEK_SET);
    PersistentState ps;
    int r = read(node_wal_fd, &ps, sizeof(ps));
    if (r != sizeof(ps))
    {
        raft_node.term = 0;
        raft_node.votedFor = -1;
        raft_node.commitIndex = -1;
        vprint_error("Loaded persistent state: header missing or short; init defaults, Node: %d\n", raft_node.id);
        return;
    }

    raft_node.term = ps.term;
    raft_node.votedFor = ps.votedFor;
    raft_node.commitIndex = ps.commitIndex;

    vprint_success("Loaded persistent state: term = %d votedFor = %d commitIndex = %d ,Node : %d\n",
                   ps.term, ps.votedFor, ps.commitIndex, raft_node.id);
}

int append_log_entry_and_persist(const LogEntry *e)
{
    int idx = -1;
    idx = wal_append_entry(node_wal_fd, e, &idx);
    return idx;
}

void raft_replay_committed_logs()
{
    if (raft_node.commitIndex < 0)
    {
        raft_node.lastApplied = -1;
        return;
    }

    int max_to_apply = raft_node.commitIndex;
    if (max_to_apply > raft_node.log_count - 1)
        max_to_apply = raft_node.log_count - 1;

    for (int i = 0; i <= max_to_apply; i++)
    {
        apply_log_entry(&raft_node.log[i]);
        raft_node.lastApplied = i;
    }
}

void *raft_thread_func(void *arg)
{
    (void)arg;
    int raft_fd = start_server(raft_node.port);
    if (raft_fd < 0)
    {
        print_error("Raft server start failed\n");
        exit(1);
    }

    // vprint_success("Raft Node ID: %d -> Raft server on %d\n", raft_node.id, raft_node.port);
    handle_raft_polls(raft_fd);
    return NULL;
}

void *server_thread_func(void *arg)
{
    (void)arg;
    int server_port = raft_node.port + 1000;
    int sfd = start_server(server_port);
    if (sfd < 0)
    {
        print_error("Client server start failed\n");
        exit(1);
    }

    //  vprint_success("Client Server started on %d\n", server_port);
    handle_server_polls(sfd);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        vprint_error("Usage: %s <id> <port> <walfile>\n", argv[0]);
        return 1;
    }
    srand(getpid());
    raft_node.id = atoi(argv[1]);
    raft_node.port = atoi(argv[2]);
    strncpy(raft_node.wal_name, argv[3], sizeof(raft_node.wal_name) - 1);

    raft_node.role = FOLLOWER;
    raft_node.votedFor = -1;
    raft_node.leader_id = -1;
    raft_node.log_count = 0;
    raft_node.commitIndex = -1;
    raft_node.lastApplied = -1;

    for (int i = 0; i < LOG_CAPACITY; i++)
        memset(&raft_node.log[i], 0, sizeof(LogEntry));

    queue_init(&command_queue);

    node_wal_fd = wal_init(raft_node.wal_name);
    if (node_wal_fd < 0)
    {
        print_error("wal init failed\n");
        return 1;
    }

    load_persistent_state();
    wal_load_all();
    raft_replay_committed_logs();

    pthread_t raft_thread, server_thread;

    pthread_create(&raft_thread, NULL, raft_thread_func, NULL);
    pthread_create(&server_thread, NULL, server_thread_func, NULL);

    pthread_join(raft_thread, NULL);
    pthread_join(server_thread, NULL);

    return 0;
}
