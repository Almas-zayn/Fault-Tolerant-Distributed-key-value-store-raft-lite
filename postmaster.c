#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ui/colors.h"

#define NODES 5

int main()
{
    system("clear");
    printf("   \n\n  \033[48;5;255m\033[30m                                                                                              \n\033[0m");
    printf("  \033[48;5;255m\033[38;5;208m                ------>   %s   <------                   \n\033[0m", "Raft Cluster (Starting 5 raft Nodes)   ");
    printf("  \033[48;5;255m\033[30m                                                                                              \n\n\033[0m");

    int pids[NODES];
    print_header("  Node ID  |  Raft Server | Client Server |       wal log name        |", YELLOW);
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
            vprint_success("%5s      | %7s      | %7d       |     %7s    |", idbuf, portbuf, 6000 + id, walbuf);
            if (node == NODES - 1)
            {
                printf("  ------------------------------------------------------------------------------------\n\n\n");
            }
            execlp("./build/raft_node", "raft_node", idbuf, portbuf, walbuf, NULL);
            _exit(1);
        }
    }

    for (int i = 0; i < NODES; i++)
        wait(NULL);
    print_success("all rafts are done\n");
    return 0;
}
