# Ledger

**A crash-fault-tolerant distributed payment ledger, built on Raft consensus.**
Go · Raft (from scratch) · Ed25519 signatures · Protobuf over TCP · 5-node cluster

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
| Networking | Length-prefixed protobuf over TCP, reconnect with backoff, bounded per-peer queues, simulated partitions |
| Testing | Deterministic Raft simulator (thousands of seeds), fuzzing, race detector, Jepsen-style checker, chaos testing |

## Architecture

```
ledger-cli ──▶ client handlers ──┐
                                 ▼
peers ◀──▶ transport ◀──▶  NODE LOOP (one goroutine)  ◀── 10ms ticker
                           ├── RaftCore (pure logic, no I/O)
                           ├── Ledger state machine
                           └── Storage (WAL, metadata, snapshots)
```

One goroutine owns all Raft and ledger state; everything else talks to it over channels, so the core needs no locks. The Raft core does no I/O at all, which lets it be tested in a deterministic simulator before any network code exists.

## Project layout

```
cmd/            ledger-node, ledger-cli, ledger-harness binaries
internal/raft   pure Raft core          internal/sim       deterministic simulator
internal/ledger state machine           internal/storage   WAL, metadata, snapshots
internal/transport TCP + framing        internal/node      node loop, client + admin services
internal/client CLI client library      internal/harness   cluster control, faults, workload, chaos
internal/checker Jepsen-style checker   proto/             protobuf definitions
```

## Quickstart *(planned)*

```bash
make build                                  # builds bin/ledger-node, bin/ledger-cli, bin/ledger-harness
./bin/ledger-harness cluster up             # start 5 nodes on localhost
./bin/ledger-cli create_account alice
./bin/ledger-cli fund alice 100
./bin/ledger-cli create_account bob
./bin/ledger-cli transfer alice bob 30
./bin/ledger-cli status -all                # roles, terms, commit index, state hash per node
./bin/ledger-harness demo leader-failure    # kill the leader under load
```

## Demos *(planned)*

1. **Double-spend prevention:** two conflicting transfers with the same nonce sent to different nodes at once; exactly one is applied.
2. **Leader failure:** the leader is killed under load; a new one takes over, and no confirmed transfer is lost or applied twice.
3. **Network partition:** the leader is cut off in a 2-node minority; the majority keeps working, and all nodes converge after healing.

## Results *(planned)*

Commit latency (p50/p95/p99) vs load, effect of batching and fsync policy, failover time, and recovery time with vs without snapshots.

## Team

- Divyanshu Jain (2025201059)
- Nikhil Patidar (2025201081)

The full design and week-by-week plan are in [`project_details.md`](project_details.md).
