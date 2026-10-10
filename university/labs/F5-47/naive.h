// naive.h - DS403 cluster kernel: the "lowest live id is the leader" design (F5-47 forensic).
// Each node trusts its own membership view; no majority is ever asked. Do not copy this.
#pragma once
#include <stdint.h>
#include "msg.h"
#include "sm.h"

void naive_init();
void naive_tick();
void naive_handle(const Msg& m);
void naive_client(Entry e);
