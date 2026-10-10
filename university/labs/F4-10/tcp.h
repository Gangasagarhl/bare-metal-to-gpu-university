// tcp.h - DR302 F4-10: a small TCP (RFC 9293) for the lab kernel: active and passive open,
// in-order delivery, cumulative ACKs, retransmission timeout per RFC 6298 with Karn's rule,
// fast retransmit on three duplicate ACKs, Reno-style congestion window, orderly close.
// Not implemented: window scaling, SACK, out-of-order queueing, urgent data, keep-alive.
#pragma once
#include <stdint.h>
#include "net.h"

namespace tcp {
enum class State { Closed, Listen, SynSent, SynReceived, Established, FinWait1, FinWait2, CloseWait,
                   Closing, LastAck, TimeWait };
const char* state_name(State s);

struct Stats {
    uint32_t segs_out, segs_in, bytes_out, bytes_in, rto_retransmits, fast_retransmits, dup_acks,
             out_of_order, old_duplicates, trimmed, rto_backoffs_max;
};

struct Conn;
Conn* connect(net::Ip dst, uint16_t port, uint32_t timeout_ms);
Conn* accept(uint16_t port, uint32_t timeout_ms);          // listen, wait for one connection
// Queues data (blocks, polling the network, until all of it is in the send buffer).
int send(Conn* c, const void* data, uint32_t len, uint32_t timeout_ms);
// Returns bytes read (> 0), 0 at end of stream (peer sent FIN), -1 on timeout or reset.
int recv(Conn* c, void* buf, uint32_t max, uint32_t timeout_ms);
// Sends FIN after all queued data and waits until the connection is closed.
bool close(Conn* c, uint32_t timeout_ms);
State state(const Conn* c);
const Stats& stats(const Conn* c);
void print(const Conn* c, const char* label);              // a one-line state dump
void reset_all();

// Called by the IP layer and by net::poll().
void input(net::Ip src, net::Ip dst, const uint8_t* seg, uint16_t len);
void timers();
void set_trace(bool on);                                   // log every retransmission
}
