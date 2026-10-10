// member.h - DS403 cluster kernel: membership by heartbeats and timeouts.
#pragma once
#include <stdint.h>
#include "msg.h"

void member_init(uint8_t nodes, uint32_t period, uint32_t timeout);
void member_tick();                       // call often: sends heartbeats, updates the view
void member_heard(const Msg& m);          // call for every message received (any message counts)
uint32_t member_view();                   // bit n set = node n is believed alive (self included)
uint32_t member_view_number();            // grows by one at each change
uint8_t member_nodes();                   // how many nodes the cluster has (from "nodes=")
uint8_t member_count(uint32_t view);      // number of bits set
void member_format(uint32_t view, char* out);   // "{1,2,3}"
