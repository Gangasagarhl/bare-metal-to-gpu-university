// ssi.h - DS403 cluster kernel: two single-system-image services (F5-44).
//   cps  - one process list for the whole cluster, names "node.pid";
//   rrun - run a job on another node and get the answer back as if it ran here.
#pragma once
#include <stdint.h>
#include "msg.h"

void ssi_handle(const Msg& m);               // serve PS_REQ / RUN_REQ, collect replies
void ssi_tick();                             // timeouts of outstanding requests
void ssi_cps_start();                        // ask every node for its process list
void ssi_rrun(uint32_t job, uint32_t n);     // place a "count primes <= n" job on a node
uint32_t count_primes(uint32_t n);           // the job itself (trial division)
