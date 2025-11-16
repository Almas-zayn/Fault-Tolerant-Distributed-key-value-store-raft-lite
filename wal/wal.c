#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "wal.h"
#include "../raft/raft_node.h"

static off_t header_size()
{
    return sizeof(PersistentState);
}

int wal_init(const char *fname)
{
    int fd = open(fname, O_RDWR | O_CREAT, 0644);
    if (fd < 0)
    {
        printf("wal open error\n");
        return -1;
    }

    struct stat st;
    fstat(fd, &st);

    if (st.st_size < header_size())
    {
        PersistentState ps;
        memset(&ps, 0, sizeof(ps));
        ps.term = 0;
        ps.votedFor = -1;
        ps.commitIndex = -1;
        write(fd, &ps, sizeof(ps));
        fsync(fd);
        printf("wal created new header\n");
    }
    else
    {
        printf("wal loaded existing\n");
    }

    return fd;
}

void wal_load_all()
{
    off_t hdr = header_size();
    struct stat st;
    fstat(node_wal_fd, &st);

    off_t data_len = st.st_size - hdr;
    if (data_len <= 0)
    {
        raft_node.log_count = 0;
        printf("No log entries\n");
        return;
    }

    int entries = data_len / sizeof(LogEntry);
    lseek(node_wal_fd, hdr, SEEK_SET);

    for (int i = 0; i < entries; i++)
    {
        LogEntry e;
        int r = read(node_wal_fd, &e, sizeof(LogEntry));
        if (r != sizeof(LogEntry))
            break;
        raft_node.log[i] = e;
    }

    raft_node.log_count = entries;
    printf("wal loaded %d entries\n", entries);
}

int wal_append_entry(int fd, const LogEntry *e, int *out_index)
{
    off_t sz = lseek(fd, 0, SEEK_END);
    if (sz == (off_t)-1)
        return -1;

    off_t hdr = header_size();
    int index = (int)((sz - hdr) / sizeof(LogEntry));

    int w = write(fd, e, sizeof(LogEntry));
    if (w != sizeof(LogEntry))
        return -1;

    fsync(fd);

    if (out_index)
        *out_index = index;
    printf("wal append index=%d\n", index);
    return index;
}

int wal_truncate_from(int fd, int index)
{
    off_t hdr = header_size();
    off_t new_size = hdr + (index * sizeof(LogEntry));

    if (ftruncate(fd, new_size) < 0)
    {
        printf("wal truncate error\n");
        return -1;
    }

    fsync(fd);
    printf("wal truncated to index=%d\n", index);
    return 0;
}
