#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "rpc/rpc.h"
#include "ui/colors.h"

static int open_conn(int port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(s);
        return -1;
    }

    return s;
}

static int send_and_recv(int s, kv_req *req, kv_res *res)
{
    int w = write(s, req, sizeof(*req));
    if (w != sizeof(*req))
        return 0;

    int r = read(s, res, sizeof(*res));
    if (r != sizeof(*res))
        return 0;

    return 1;
}

static int ensure_connected(int *sock, int port)
{
    if (*sock >= 0)
        return 1;
    *sock = open_conn(port);
    return (*sock >= 0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        vprint_error("Usage: %s <client_port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int sock = -1;

    vprint_info("KV Client started. Connecting to %d...\n", port);

    if (!ensure_connected(&sock, port))
    {
        printf("Initial connection failed.\n");
        return 1;
    }

    vprint_success("Connected to server %d\n", port);

    char cmd[256];

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin))
            break;
        if (strncmp(cmd, "exit", 4) == 0)
            break;

        char op[16], key[64], val[128];
        memset(op, 0, sizeof(op));
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));

        int count = sscanf(cmd, "%s %s %s", op, key, val);
        if (count < 2)
        {
            print_error("invalid\n");
            continue;
        }

        kv_req req;
        kv_res res;
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        if (strcasecmp(op, "PUT") == 0)
        {
            if (count < 3)
            {
                print_error("usage: PUT key value\n");
                continue;
            }
            req.req_type = PUT;
            strncpy(req.key, key, MAX_KEY_LEN - 1);
            strncpy(req.value, val, MAX_VAL_LEN - 1);
        }
        else if (strcasecmp(op, "GET") == 0)
        {
            req.req_type = GET;
            strncpy(req.key, key, MAX_KEY_LEN - 1);
        }
        else if (strcasecmp(op, "DEL") == 0)
        {
            req.req_type = DEL;
            strncpy(req.key, key, MAX_KEY_LEN - 1);
        }
        else
        {
            print_error("unknown cmd\n");
            continue;
        }

    retry:
        if (!ensure_connected(&sock, port))
        {
            print_error("connection lost. retrying...\n");
            goto retry;
        }

        if (!send_and_recv(sock, &req, &res))
        {
            print_error("connection error. reconnecting...\n");
            close(sock);
            sock = -1;
            goto retry;
        }

        if (res.status == 1)
        {
            if (req.req_type == GET)
                vprint_success("GET %s => %s\n", req.key, res.value);
            else if (req.req_type == PUT)
                vprint_success("PUT committed\n");
            else if (req.req_type == DEL)
                vprint_success("DEL committed\n");
        }
        else
        {
            if (res.leader_id > 0)
            {
                int new_port = 6000 + res.leader_id;
                vprint_info("Not leader. Redirecting to leader %d...\n", res.leader_id);

                close(sock);
                sock = -1;
                port = new_port;

                vprint_info("Connecting to leader on %d...\n", port);
                goto retry;
            }

            if (req.req_type == GET)
                vprint_info("GET %s => not_found\n", req.key);
            else
                print_error("operation failed\n");
        }
    }

    if (sock >= 0)
        close(sock);
    return 0;
}
