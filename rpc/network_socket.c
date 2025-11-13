#include <unistd.h>
#include <arpa/inet.h>
#include "rpc.h"
#include "../raft/raft_node.h"

int start_server(int port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return -1;
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(s);
        return -1;
    }
    if (listen(s, 20) < 0)
    {
        close(s);
        return -1;
    }
    return s;
}
int send_raft_rpc_req(raft_req req, RaftNode node)
{
    for (int peer = 1; peer <= NODES; ++peer)
    {
        if (peer == node.id)
            continue;
        int peer_port = node.port + (peer - node.id);
        connect_and_send(peer_port, &req, sizeof(req));
    }
    return 0;
}

int connect_and_send(int port, void *buf, size_t len)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, IP_ADDR, &addr.sin_addr);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(s);
        return -1;
    }
    ssize_t w = write(s, buf, len);
    close(s);
    return (w == len) ? 0 : -1;
}

int send_raft_rpc_res(raft_res res, RaftNode node)
{
    int peer = res.id;
    int peer_port = node.port + (peer - node.id);
    connect_and_send(peer_port, &res, sizeof(res));
    return 0;
}

int send_kv_rpc_res(kv_req req)
{
}