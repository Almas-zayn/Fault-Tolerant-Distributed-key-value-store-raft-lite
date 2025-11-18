# Fault-Tolerant Distributed Key-Value Store – Raft Lite

A lightweight implementation of a fault-tolerant distributed key-value store built using the Raft consensus algorithm.


## 🔷 What is Raft?

Raft is a **distributed consensus algorithm** used to maintain a replicated log across multiple servers. It ensures that a cluster of machines agrees on the same sequence of operations, even if some machines fail.

### Key Concepts:

* **Leader Election** – A single leader manages the cluster; followers replicate the leader’s log.
* **Log Replication** – Client commands are appended to the leader log and replicated to followers.
* **Fault Tolerance** – System continues even if some nodes crash, as long as a majority stay alive.
* **Ease of Understanding** – Raft is designed to be simpler to understand compared to Paxos.

---

## 🔷 Project Overview

This project implements a distributed key-value store backed by the Raft protocol.

### Features:

* Cluster-based key-value storage
* Leader election and heartbeat mechanism
* Log replication using Raft RPCs
* Write-Ahead Logging (WAL) for persistence
* Simple CLI client to interact with the cluster

The project is educational, helping understand how distributed consensus can be applied to build fault-tolerant systems.

---

## 📁 Repository Structure

A clean breakdown of the project layout:

```
Fault-Tolerant-Distributed-key-value-store-raft-lite/
│
├── build/               # Build scripts, binaries, Makefile outputs
│
├── raft/                # Core Raft implementation
│   ├── raft_node.c
│   ├── raft_node.h
│   ├── handle_server.c
│   ├── replicate_entries.c
│   ├── raft_helpers.c
│   ├── raft_helpers.h
│   ├── cmd_queue.h
│   ├── cmd_queue.c
│   └── handle_raft.c
│
├── rpc/                 # RPC communication (AppendEntries, RequestVote)
│   ├── network_socket.c
│   └── rpc.h
│
├── wal/                 # Write-Ahead Log persistence module
│   ├── wal.c
│   └── wal.h
│
├── wal-logs/            # Actual WAL log files generated at runtime
│
├── ui/                  # Optional UI/monitoring utilities
│   ├── ui.c
│   └── colors.h
│
├── kv_client           # CLI tool to interact with the distributed store
├── kv_client.c 
│
├── postmaster.c         # Main server node entry point
├── makefile             # Build script
└── README.md            # Project documentation
```

---

## 🚀 Getting Started

### **1. Clone the Repository**

```bash
git clone https://github.com/Almas-zayn/Fault-Tolerant-Distributed-key-value-store-raft-lite.git
cd Fault-Tolerant-Distributed-key-value-store-raft-lite
```

### **2. Build the Project**

```bash
make
```

### **3. Start Multiple Nodes**

Run these in separate terminals:

```bash
./build/postmaster 
```

### **4. Use the Key-Value Client**

```bash
./kv_client <port> 
```

---


If the leader fails, a follower will automatically take over.

---

## ⚙️ How It Works (Internal Flow)

### **1. Leader Election**

* Nodes start as followers.
* If no heartbeat received → convert to candidate.
* Candidate requests votes.
* Majority votes → becomes leader.

### **2. Log Replication**

* Leader receives commands from the client.
* Appends to its log.
* Sends *AppendEntries RPC* to followers.
* When majority stores → commit.

### **3. Fault Tolerance**

* System works as long as majority nodes are alive.
* WAL ensures recovery after crashes.
* Logs always converge to a consistent state.


---

If you found this project useful, consider ⭐ starring the repository!
