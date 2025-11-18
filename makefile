CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I./raft -I./rpc -I./wal

BUILD_DIR = build

POSTMASTER_SRC = postmaster.c \
                  ui/ui.c
				  
RAFT_NODE_SRC = \
	raft/raft_node.c \
	raft/handle_raft.c \
	raft/raft_helpers.c \
	raft/replicate_entries.c \
	raft/handle_server.c \
	raft/cmd_queue.c \
	rpc/network_socket.c \
	hash-kv-store/hash_kv_store.c \
	wal/wal.c \
	ui/ui.c

POSTMASTER_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(POSTMASTER_SRC))

RAFT_NODE_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RAFT_NODE_SRC))

all: $(BUILD_DIR)/postmaster $(BUILD_DIR)/raft_node

$(BUILD_DIR)/postmaster: $(POSTMASTER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(POSTMASTER_OBJS)

$(BUILD_DIR)/raft_node: $(RAFT_NODE_OBJS)
	$(CC) $(CFLAGS) -o $@ $(RAFT_NODE_OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all
