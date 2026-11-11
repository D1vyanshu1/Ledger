# Ledger

**A crash-fault-tolerant distributed payment ledger, built on Raft consensus.**
C++20 · Raft (from scratch) · Ed25519 signatures (libsodium) · Protobuf over TCP (epoll) · 5-node cluster

> **Status: in progress.** This README describes the design. Sections marked *(planned)* will be filled in as each part is built and tested.

## What it is

Ledger runs on 5 nodes and keeps working when up to 2 of them crash.

- Clients create accounts and send **Ed25519-signed transfers** to any node; followers redirect to the leader.
- The leader replicates each transaction with **Raft**, implemented from scratch. An entry commits once a majority (3 of 5) has stored it on disk.
- Every node applies committed entries in the same order to a **deterministic state machine**, so all nodes end up with identical balances.
- **Per-account nonces** block replays and double-spends. Clients retry with the same signed bytes, so a transfer is applied **exactly once**, even across a leader change.
- A **CRC-checked write-ahead log**, snapshots and log compaction let any node recover after `kill -9`.

**Fault model:** nodes may crash and restart; messages may be lost, delayed or partitioned. Byzantine (malicious) nodes and membership changes are out of scope.

## Features

| Area | What's included |
|---|---|
| Consensus | Leader election, log replication, Figure 8-safe commit rule, pre-vote, ReadIndex linearizable reads, fast log backtracking |
| Ledger | Signed `CREATE_ACCOUNT` / `TRANSFER`, canonical signing bytes, nonces, genesis faucet, constant total supply |
| Storage | Segmented WAL with CRC32C, group-commit fsync, atomic metadata, snapshots, log compaction, InstallSnapshot |
| Networking | Single-threaded epoll event loop, length-prefixed protobuf over TCP, reconnect with backoff, bounded per-peer buffers, simulated partitions and slow/lossy links |
| Testing | Deterministic Raft simulator (thousands of seeds), sanitizers, fuzzing, Jepsen-style checker, chaos testing |

## Architecture

```
ledger-cli ──▶ client connections ──┐
                                    ▼
peers ◀──▶ transport ◀──▶  EVENT LOOP (one thread, epoll)  ◀── timerfd 10ms
                           ├── RaftCore (pure logic, no I/O)
                           ├── Ledger state machine
                           └── Storage (WAL, metadata, snapshots)
```

Each node is a single thread: sockets, the tick timer and client requests are all file descriptors multiplexed by `epoll`, so there are no locks and no data races. The Raft core does no I/O at all, which lets it be tested in a deterministic simulator before any socket code exists.

## Project layout

```
src/raft      pure Raft core             tests/sim        deterministic simulator
src/ledger    state machine              src/storage      WAL, metadata, snapshots
src/net       epoll loop, TCP, framing   src/node         event loop body, client + admin services
src/client    CLI client library         src/apps         ledger-node, ledger-cli, ledger-bench
harness/      Python: cluster control, faults, checker, chaos, demos, plots
proto/        protobuf definitions       config/          cluster + genesis files
```

## Quickstart *(planned)*

```bash
sudo apt install build-essential cmake ninja-build protobuf-compiler libprotobuf-dev \
                 libsodium-dev nlohmann-json3-dev libgtest-dev python3-matplotlib python3-protobuf
make build                                  # builds build/release/{ledger-node,ledger-cli,ledger-bench}
make cluster-up                             # start 5 nodes on localhost
./build/release/ledger-cli create_account alice
./build/release/ledger-cli fund alice 100
./build/release/ledger-cli create_account bob
./build/release/ledger-cli transfer alice bob 30
./build/release/ledger-cli status --all     # roles, terms, commit index, state hash per node
python3 -m harness demo leader-failure      # kill the leader under load
```

## Demos *(planned)*

1. **Double-spend prevention:** two conflicting transfers with the same nonce sent to different nodes at once; exactly one is applied.
2. **Leader failure:** the leader is killed under load; a new one takes over, and no confirmed transfer is lost or applied twice.
3. **Network partition:** the leader is cut off in a 2-node minority; the majority keeps working, and all nodes converge after healing.

## Results *(planned)*

Commit latency (p50/p95/p99) vs load, effect of batching and fsync policy, failover time, cluster size (3 vs 5), and recovery time with vs without snapshots.

## Team

- Divyanshu Jain (2025201059)
- Nikhil Patidar (2025201081)

The full design and phase-by-phase plan are in [`project_details.md`](project_details.md).
