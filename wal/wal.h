#ifndef WAL_H
#define WAL_H

#include <unistd.h>
#include "../rpc/rpc.h"
#include "../raft/raft_node.h"

int wal_init(const char *fname);
void wal_load_all();
int wal_append_entry(int fd, const LogEntry *e, int *out_index);
int wal_truncate_from(int fd, int index);

#endif
