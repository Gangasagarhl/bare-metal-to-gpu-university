// raft.h - DS403 cluster kernel: a small Raft (leader election and log replication) and the
// cluster scheduler that runs on top of it (F5-47). Not included, on purpose: persistence
// of term/vote/log (nodes here never restart), snapshots, membership changes (DS302).
#pragma once
#include <stdint.h>
#include "msg.h"
#include "sm.h"

void raft_init(uint8_t nodes);
void raft_tick();                         // elections, heartbeats, commit, apply, scheduler
void raft_handle(const Msg& m);
bool raft_is_leader();
// A client command from this node. Done when it is applied (ok or not) or after `wait`
// ticks without an answer; the result is printed by the client code in raft.cc.
void raft_client(Entry e, uint32_t wait);
uint32_t count_primes_upto(uint32_t n);    // the jobs: count the primes <= n
