// tcp.cc - DR302 F4-10: the TCP state machine, send and receive buffers, timers.
// Behaviour follows RFC 9293 (TCP), RFC 6298 (retransmission timer) and RFC 5681
// (congestion control) as remembered by the author; the documents were not opened in this
// build. Deviations chosen for the lab are marked "LAB:".
#include "tcp.h"
#include "../F4-01/kbase.h"
#include "../F4-08/intr.h"

namespace tcp {
constexpr uint32_t SBUF = 65536, RBUF = 32768, MSS = 1460;
struct Conn {
    bool used;
    tcp::State st;
    net::Ip rip;
    uint16_t lport, rport;
    uint32_t iss, snd_una, snd_nxt, snd_max, snd_wnd, peer_mss;
    uint32_t rcv_nxt;
    // send buffer: data byte k (k = 0, 1, ...) has sequence number iss + 1 + k
    uint8_t sbuf[SBUF];
    uint32_t queued;                       // data bytes ever queued
    bool fin_queued, fin_sent;
    uint8_t rbuf[RBUF];
    uint32_t r_head, r_len;
    bool peer_fin, reset;
    // timers and estimators
    bool rto_armed;
    uint64_t rto_deadline, tw_deadline;
    uint32_t rto, srtt, rttvar;
    bool have_rtt, timing;
    uint32_t rtt_seq;
    uint64_t rtt_start;
    uint32_t cwnd, ssthresh, dupacks, rexmits;
    tcp::Stats s;
};
}  // namespace tcp

namespace {
using namespace net;
using tcp::State;
constexpr uint8_t FIN = 0x01, SYN = 0x02, RST = 0x04, PSH = 0x08, ACK = 0x10;
using tcp::SBUF;
using tcp::RBUF;
using tcp::MSS;
constexpr uint32_t RTO_INITIAL = 1000, RTO_MAX = 60000;
constexpr uint32_t RTO_MIN = 200;          // LAB: RFC 6298 asks for 1 s; 200 ms keeps runs short
constexpr uint32_t TIME_WAIT_MS = 200;     // LAB: 2 x MSL would be minutes

using tcp::Conn;

Conn g_conn[2];
bool g_trace = false;
uint32_t g_next_port = 49152, g_iss = 0x1000;

bool lt(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b) < 0; }
bool le(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b) <= 0; }
uint32_t min32(uint32_t a, uint32_t b) { return a < b ? a : b; }
uint32_t now() { return static_cast<uint32_t>(clock::ms()); }

uint32_t data_seq(const Conn& c, uint32_t k) { return c.iss + 1 + k; }
uint32_t fin_seq(const Conn& c) { return data_seq(c, c.queued); }

void arm(Conn& c) { c.rto_armed = true; c.rto_deadline = clock::ms() + c.rto; }

void output(Conn& c, uint32_t seq, uint8_t flags, uint32_t len)
{
    uint8_t seg[20 + 4 + MSS];
    const uint8_t hl = (flags & SYN) ? 24 : 20;
    put16(seg, c.lport);
    put16(seg + 2, c.rport);
    put32(seg + 4, seq);
    put32(seg + 8, (flags & ACK) ? c.rcv_nxt : 0);
    seg[12] = static_cast<uint8_t>((hl / 4) << 4);
    seg[13] = flags;
    put16(seg + 14, static_cast<uint16_t>(min32(RBUF - c.r_len, 65535)));   // receive window
    put16(seg + 16, 0);
    put16(seg + 18, 0);
    if (flags & SYN) { seg[20] = 2; seg[21] = 4; put16(seg + 22, MSS); }    // MSS option
    for (uint32_t i = 0; i < len; ++i) {                                     // gather from the ring
        const uint32_t k = seq - c.iss - 1 + i;
        seg[hl + i] = c.sbuf[k % SBUF];
    }
    const uint16_t total = static_cast<uint16_t>(hl + len);
    uint8_t pseudo[12];
    put32(pseudo, cfg.addr);
    put32(pseudo + 4, c.rip);
    pseudo[8] = 0;
    pseudo[9] = PROTO_TCP;
    put16(pseudo + 10, total);
    put16(seg + 16, checksum(seg, total, sum16(pseudo, 12, 0)));
    if ((len || (flags & FIN)) && loss_tx() && lose(seq ^ (c.rexmits * 0x9E3779B9u), loss_tx())) {
        ++stats.tx_dropped_loss;          // induced loss: the segment never reaches the wire
        if (g_trace) kprintf("tcp: t=%u dropped (induced) seq %u len %u attempt %u\n", now(), seq - c.iss, len, c.rexmits);
        return;
    }
    ++c.s.segs_out;
    c.s.bytes_out += len;
    ip_send(c.rip, PROTO_TCP, seg, total);
}

void send_ack(Conn& c) { output(c, c.snd_nxt, ACK, 0); }

// Sends as much new (or, after a timeout, re-sent) data as the windows allow.
void try_send(Conn& c)
{
    if (c.st != State::Established && c.st != State::CloseWait && c.st != State::FinWait1 &&
        c.st != State::LastAck && c.st != State::Closing)
        return;
    for (;;) {
        const uint32_t flight = c.snd_nxt - c.snd_una;
        uint32_t wnd = min32(c.snd_wnd, c.cwnd);
        if (wnd == 0 && flight == 0) wnd = 1;                 // zero-window probe: one byte
        const uint32_t next = c.snd_nxt - c.iss - 1;          // data index of snd_nxt
        const uint32_t avail = c.queued > next ? c.queued - next : 0;
        if (avail && flight < wnd) {
            const uint32_t len = min32(min32(avail, c.peer_mss), wnd - flight);
            const bool fresh = c.snd_nxt == c.snd_max;
            if (fresh && !c.timing) { c.timing = true; c.rtt_seq = c.snd_nxt; c.rtt_start = clock::ms(); }
            output(c, c.snd_nxt, ACK | PSH, len);
#ifdef F410_TIMER_BUG
            if (fresh && !c.rto_armed) arm(c);                // colleague: "arm when sending new data"
#else
            if (!c.rto_armed) arm(c);
#endif
            c.snd_nxt += len;
            if (lt(c.snd_max, c.snd_nxt)) c.snd_max = c.snd_nxt;
            continue;
        }
        if (c.fin_queued && avail == 0 && c.snd_nxt == fin_seq(c) && flight < wnd + 1) {
            output(c, c.snd_nxt, FIN | ACK, 0);
            c.fin_sent = true;
            c.snd_nxt += 1;
            if (lt(c.snd_max, c.snd_nxt)) c.snd_max = c.snd_nxt;
            if (!c.rto_armed) arm(c);
            if (c.st == State::Established) c.st = State::FinWait1;
            else if (c.st == State::CloseWait) c.st = State::LastAck;
        }
        return;
    }
}

void rtt_sample(Conn& c, uint32_t r)
{
    if (!c.have_rtt) {                       // RFC 6298 (2.2)
        c.srtt = r;
        c.rttvar = r / 2;
        c.have_rtt = true;
    } else {                                 // (2.3): beta = 1/4, alpha = 1/8
        const uint32_t diff = c.srtt > r ? c.srtt - r : r - c.srtt;
        c.rttvar = (3 * c.rttvar + diff) / 4;
        c.srtt = (7 * c.srtt + r) / 8;
    }
    uint32_t rto = c.srtt + (4 * c.rttvar > 1 ? 4 * c.rttvar : 1);
    if (rto < RTO_MIN) rto = RTO_MIN;
    if (rto > RTO_MAX) rto = RTO_MAX;
    c.rto = rto;
}

void on_ack(Conn& c, uint32_t ack, uint16_t wnd, uint16_t len, uint8_t flags)
{
    if (lt(c.snd_una, ack) && le(ack, c.snd_max)) {
        const uint32_t acked = ack - c.snd_una;
        c.snd_una = ack;
        if (lt(c.snd_nxt, c.snd_una)) c.snd_nxt = c.snd_una;
        if (c.timing && lt(c.rtt_seq, ack)) {           // Karn: only segments sent once are timed
            rtt_sample(c, static_cast<uint32_t>(clock::ms() - c.rtt_start));
            c.timing = false;
        }
        if (c.dupacks >= 3) c.cwnd = c.ssthresh;        // leave fast recovery
        else if (c.cwnd < c.ssthresh) c.cwnd += min32(acked, c.peer_mss);   // slow start
        else c.cwnd += c.peer_mss * c.peer_mss / c.cwnd;                    // congestion avoidance
        c.dupacks = 0;
        c.rexmits = 0;
        c.snd_wnd = wnd;
#ifdef F410_TIMER_BUG
        c.rto_armed = false;                            // colleague: "the next send re-arms it"
#else
        if (c.snd_una == c.snd_max) c.rto_armed = false; // everything acknowledged
        else arm(c);                                      // RFC 6298 (5.3): restart for the rest
#endif
        if (c.fin_sent && c.snd_una == fin_seq(c) + 1) { // our FIN is acknowledged
            if (c.st == State::FinWait1) c.st = State::FinWait2;
            else if (c.st == State::Closing) { c.st = State::TimeWait; c.tw_deadline = clock::ms() + TIME_WAIT_MS; }
            else if (c.st == State::LastAck) c.st = State::Closed;
        }
        return;
    }
    if (ack == c.snd_una && len == 0 && !(flags & (SYN | FIN)) && c.snd_max != c.snd_una && wnd == c.snd_wnd) {
        ++c.s.dup_acks;
        if (++c.dupacks == 3) {                         // RFC 5681 fast retransmit
            const uint32_t flight = c.snd_max - c.snd_una;
            c.ssthresh = flight / 2 > 2 * c.peer_mss ? flight / 2 : 2 * c.peer_mss;
            ++c.rexmits;
            ++c.s.fast_retransmits;
            const uint32_t k = c.snd_una - c.iss - 1;
            const uint32_t l = min32(c.peer_mss, c.queued - k);
            if (g_trace) kprintf("tcp: t=%u 3 duplicate ACKs: fast retransmit seq %u len %u\n", now(), c.snd_una - c.iss, l);
            if (l) output(c, c.snd_una, ACK | PSH, l);
            c.cwnd = c.ssthresh + 3 * c.peer_mss;
        } else if (c.dupacks > 3) {
            c.cwnd += c.peer_mss;                       // inflate during recovery
        }
    }
    c.snd_wnd = wnd;
}

Conn* find(Ip src, uint16_t sport, uint16_t dport)
{
    for (Conn& c : g_conn)
        if (c.used && c.st != State::Listen && c.rip == src && c.rport == sport && c.lport == dport) return &c;
    for (Conn& c : g_conn)
        if (c.used && c.st == State::Listen && c.lport == dport) return &c;
    return nullptr;
}

Conn* alloc()
{
    for (Conn& c : g_conn) {
        if (!c.used || c.st == State::Closed) {
            memset(&c, 0, sizeof c);
            c.used = true;
            c.rto = RTO_INITIAL;
            c.peer_mss = 536;                // RFC 9293 default when no MSS option arrives
            c.cwnd = 2 * MSS;
            c.ssthresh = 65535;
            c.iss = g_iss;
            g_iss += 0x01000000;
            return &c;
        }
    }
    return nullptr;
}

bool wait_until(Conn* c, State a, uint32_t timeout_ms)
{
    const uint64_t end = clock::ms() + timeout_ms;
    while (clock::ms() < end) {
        poll();
        if (c->st == a) return true;
        if (c->reset || c->st == State::Closed) return false;
        intr::wait();
    }
    return false;
}
}  // namespace

namespace tcp {
const char* state_name(State s)
{
    static const char* const names[] = {"CLOSED", "LISTEN", "SYN-SENT", "SYN-RECEIVED", "ESTABLISHED", "FIN-WAIT-1",
                                        "FIN-WAIT-2", "CLOSE-WAIT", "CLOSING", "LAST-ACK", "TIME-WAIT"};
    return names[static_cast<int>(s)];
}

void set_trace(bool on) { g_trace = on; }
void reset_all() { memset(g_conn, 0, sizeof g_conn); }
State state(const Conn* c) { return c->st; }
const Stats& stats(const Conn* c) { return c->s; }

void print(const Conn* c, const char* label)
{
    kprintf("%s: state %s, snd_una %u snd_nxt %u snd_max %u (relative), queued %u, rcv_nxt %u, cwnd %u, "
            "ssthresh %u, peer window %u, RTO %u ms (%s), SRTT %u ms\n", label, state_name(c->st), c->snd_una - c->iss,
            c->snd_nxt - c->iss, c->snd_max - c->iss, c->queued, c->rcv_nxt, c->cwnd, c->ssthresh, c->snd_wnd, c->rto,
            c->rto_armed ? "armed" : "NOT armed", c->srtt);
    kprintf("%s: segments out %u in %u, bytes out %u in %u, RTO retransmits %u, fast retransmits %u, "
            "duplicate ACKs %u, segments dropped: after a gap %u, already received %u, trimmed (buffer full) %u\n", label,
            c->s.segs_out, c->s.segs_in, c->s.bytes_out, c->s.bytes_in, c->s.rto_retransmits, c->s.fast_retransmits,
            c->s.dup_acks, c->s.out_of_order, c->s.old_duplicates, c->s.trimmed);
}

Conn* connect(Ip dst, uint16_t port, uint32_t timeout_ms)
{
    Conn* c = alloc();
    if (!c) return nullptr;
    c->rip = dst;
    c->rport = port;
    c->lport = static_cast<uint16_t>(g_next_port++);
    c->st = State::SynSent;
    c->snd_una = c->iss;
    c->snd_nxt = c->iss + 1;
    c->snd_max = c->snd_nxt;
    output(*c, c->iss, SYN, 0);
    arm(*c);
    if (!wait_until(c, State::Established, timeout_ms)) return nullptr;
    return c;
}

Conn* accept(uint16_t port, uint32_t timeout_ms)
{
    Conn* c = alloc();
    if (!c) return nullptr;
    c->st = State::Listen;
    c->lport = port;
    if (!wait_until(c, State::Established, timeout_ms)) return nullptr;
    return c;
}

int send(Conn* c, const void* data, uint32_t len, uint32_t timeout_ms)
{
    const auto* p = static_cast<const uint8_t*>(data);
    uint32_t done = 0;
    const uint64_t end = clock::ms() + timeout_ms;
    while (done < len) {
        if (c->reset || clock::ms() > end) return -1;
        const uint32_t acked = c->snd_una - c->iss - 1;
        const uint32_t space = SBUF - (c->queued - acked);
        const uint32_t n = min32(space, len - done);
        for (uint32_t i = 0; i < n; ++i) c->sbuf[(c->queued + i) % SBUF] = p[done + i];
        c->queued += n;
        done += n;
        try_send(*c);
        poll();
        if (done < len) intr::wait();
    }
    return static_cast<int>(done);
}

int recv(Conn* c, void* buf, uint32_t max, uint32_t timeout_ms)
{
    const uint64_t end = clock::ms() + timeout_ms;
    for (;;) {
        poll();
        if (c->r_len) {
            const uint32_t n = min32(max, c->r_len);
            auto* out = static_cast<uint8_t*>(buf);
            for (uint32_t i = 0; i < n; ++i) out[i] = c->rbuf[(c->r_head + i) % RBUF];
            const bool was_closed = RBUF - c->r_len < MSS;
            c->r_head = (c->r_head + n) % RBUF;
            c->r_len -= n;
            if (was_closed && RBUF - c->r_len >= MSS) send_ack(*c);   // window update
            return static_cast<int>(n);
        }
        if (c->peer_fin) return 0;
        if (c->reset || clock::ms() > end) return -1;
        intr::wait();
    }
}

bool close(Conn* c, uint32_t timeout_ms)
{
    c->fin_queued = true;
    try_send(*c);
    const uint64_t end = clock::ms() + timeout_ms;
    while (clock::ms() < end) {
        poll();
        if (c->st == State::Closed || c->st == State::TimeWait) return true;
        if (c->reset) return false;
        intr::wait();
    }
    return false;
}

void input(Ip src, Ip dst, const uint8_t* seg, uint16_t len)
{
    if (len < 20) return;
    uint8_t pseudo[12];
    put32(pseudo, src);
    put32(pseudo + 4, dst);
    pseudo[8] = 0;
    pseudo[9] = PROTO_TCP;
    put16(pseudo + 10, len);
    if (checksum(seg, len, sum16(pseudo, 12, 0)) != 0) { ++net::stats.rx_bad_checksum; return; }
    const uint16_t sport = be16(seg), dport = be16(seg + 2), wnd = be16(seg + 14);
    const uint32_t seq = be32(seg + 4), ack = be32(seg + 8);
    const uint8_t hl = (seg[12] >> 4) * 4, flags = seg[13];
    if (hl < 20 || hl > len) return;
    const uint16_t dlen = static_cast<uint16_t>(len - hl);
    const uint8_t* data = seg + hl;
    Conn* cp = find(src, sport, dport);
    if (!cp) return;                                  // LAB: no RST for unknown ports
    Conn& c = *cp;
    ++c.s.segs_in;
    if (flags & RST) { c.reset = true; c.st = State::Closed; kprintf("tcp: connection reset by peer\n"); return; }

    if (c.st == State::Listen) {
        if (!(flags & SYN)) return;
        c.rip = src;
        c.rport = sport;
        c.rcv_nxt = seq + 1;
        for (uint8_t i = 20; i + 3 < hl;) {           // options: look for MSS
            if (seg[i] == 0) break;
            if (seg[i] == 1) { ++i; continue; }
            if (seg[i] == 2 && seg[i + 1] == 4) c.peer_mss = be16(seg + i + 2);
            i = static_cast<uint8_t>(i + (seg[i + 1] ? seg[i + 1] : 1));
        }
        c.snd_wnd = wnd;
        c.st = State::SynReceived;
        c.snd_una = c.iss;
        c.snd_nxt = c.iss + 1;
        c.snd_max = c.snd_nxt;
        output(c, c.iss, SYN | ACK, 0);
        arm(c);
        return;
    }
    if (c.st == State::SynSent) {
        if ((flags & (SYN | ACK)) != (SYN | ACK) || ack != c.iss + 1) return;
        c.rcv_nxt = seq + 1;
        c.snd_una = ack;
        c.snd_wnd = wnd;
        for (uint8_t i = 20; i + 3 < hl;) {
            if (seg[i] == 0) break;
            if (seg[i] == 1) { ++i; continue; }
            if (seg[i] == 2 && seg[i + 1] == 4) c.peer_mss = be16(seg + i + 2);
            i = static_cast<uint8_t>(i + (seg[i + 1] ? seg[i + 1] : 1));
        }
        c.rto_armed = false;
        c.st = State::Established;
        send_ack(c);
        return;
    }
    if (c.st == State::SynReceived) {
        if (!(flags & ACK) || ack != c.iss + 1) return;
        c.snd_una = ack;
        c.snd_wnd = wnd;
        c.rto_armed = false;
        c.st = State::Established;
    }
    if (flags & ACK) on_ack(c, ack, wnd, dlen, flags);

    bool need_ack = false;
    if (dlen || (flags & FIN)) {
        if (seq != c.rcv_nxt) {                       // not the next byte we expect
            if (static_cast<int32_t>(seq - c.rcv_nxt) < 0) ++c.s.old_duplicates;   // already have it
            else ++c.s.out_of_order;                  // a gap: something before it is missing
            send_ack(c);                              // a duplicate ACK tells the sender
            return;
        }
        if (dlen && (c.st == State::Established || c.st == State::FinWait1 || c.st == State::FinWait2)) {
            const uint32_t n = min32(dlen, RBUF - c.r_len);
            for (uint32_t i = 0; i < n; ++i) c.rbuf[(c.r_head + c.r_len + i) % RBUF] = data[i];
            c.r_len += n;
            c.rcv_nxt += n;
            c.s.bytes_in += n;
            need_ack = true;
            if (n < dlen) { ++c.s.trimmed; send_ack(c); return; }   // buffer full: the rest comes again later
        }
        if (flags & FIN) {
            c.rcv_nxt += 1;
            c.peer_fin = true;
            need_ack = true;
            if (c.st == State::Established) c.st = State::CloseWait;
            else if (c.st == State::FinWait1) c.st = State::Closing;
            else if (c.st == State::FinWait2) { c.st = State::TimeWait; c.tw_deadline = clock::ms() + TIME_WAIT_MS; }
        }
    }
    if (need_ack) send_ack(c);
    try_send(c);
}

void timers()
{
    for (Conn& c : g_conn) {
        if (!c.used) continue;
        if (c.st == State::TimeWait && clock::ms() >= c.tw_deadline) { c.st = State::Closed; continue; }
        if (!c.rto_armed || clock::ms() < c.rto_deadline) continue;
        c.rto = c.rto * 2 > RTO_MAX ? RTO_MAX : c.rto * 2;   // RFC 6298 (5.5): back off
        ++c.rexmits;
        if (c.rexmits > c.s.rto_backoffs_max) c.s.rto_backoffs_max = c.rexmits;
        if (c.st == State::SynSent) { output(c, c.iss, SYN, 0); arm(c); continue; }
        if (c.st == State::SynReceived) { output(c, c.iss, SYN | ACK, 0); arm(c); continue; }
        ++c.s.rto_retransmits;
        const uint32_t flight = c.snd_max - c.snd_una;
        c.ssthresh = flight / 2 > 2 * c.peer_mss ? flight / 2 : 2 * c.peer_mss;   // RFC 5681 (4)
        c.cwnd = c.peer_mss;
        c.dupacks = 0;
        c.timing = false;                                    // Karn: do not time a retransmission
        if (g_trace)
            kprintf("tcp: t=%u retransmission timeout: resend from seq %u, RTO now %u ms\n", now(), c.snd_una - c.iss, c.rto);
        c.snd_nxt = c.snd_una;                               // go back to the oldest unacknowledged byte
        if (c.fin_sent && c.snd_una == fin_seq(c)) {         // only the FIN is outstanding
            output(c, c.snd_una, FIN | ACK, 0);
            c.snd_nxt = c.snd_una + 1;
            arm(c);
            continue;
        }
        c.rto_armed = false;
        try_send(c);                                         // sends the first segment again
        if (!c.rto_armed) arm(c);                            // RFC 6298 (5.6): restart the timer
    }
}
}  // namespace tcp
