#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define NODES 5

int main()
{
    printf("postmaster started\n");
    int pids[NODES];
    for (int node = 0; node < NODES; node++)
    {
        pids[node] = fork();
        if (pids[node] == 0)
        {
            char idbuf[8];
            char portbuf[8];
            char walbuf[32];
            int id = node + 1;
            int port = 5000 + id;
            snprintf(idbuf, sizeof(idbuf), "%d", id);
            snprintf(portbuf, sizeof(portbuf), "%d", port);
            snprintf(walbuf, sizeof(walbuf), "wal-logs/node%d.log", id);
            execlp("./build/raft_node", "raft_node", idbuf, portbuf, walbuf, NULL);
            _exit(1);
        }
    }

    for (int i = 0; i < NODES; i++)
        wait(NULL);
    printf("all rafts are done\n");
    return 0;
}