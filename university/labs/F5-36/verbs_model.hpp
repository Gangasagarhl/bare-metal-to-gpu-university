// verbs_model.hpp - DS401 F5-36: the university's "toy verbs" model.
// NOT libibverbs and NOT a NIC: an in-process model of the objects and rules this chapter
// teaches (protection domain, memory regions with lkey/rkey and access flags, queue pairs
// with a state machine, send and receive queues, a completion queue, one-sided RDMA WRITE
// and two-sided SEND/RECV, receiver-not-ready retries, errors that move a QP to the error
// state and flush its remaining work requests). The access-flag bit values and the status
// names are the ones of the installed rdma-core header <infiniband/verbs.h>; the behaviour
// is the chapter's simplified reading of the specification (see its unverified boxes).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace toy {

enum Access : unsigned { LOCAL_WRITE = 1, REMOTE_WRITE = 1u << 1, REMOTE_READ = 1u << 2 };
enum class State { RESET, INIT, RTR, RTS, ERR };
enum class Status { SUCCESS, LOC_PROT_ERR, WR_FLUSH_ERR, REM_ACCESS_ERR, RNR_RETRY_EXC_ERR };
enum class Op { SEND, RECV, RDMA_WRITE };

inline const char* name(State s)
{
    static const char* n[] = {"RESET", "INIT", "RTR", "RTS", "ERR"};
    return n[static_cast<int>(s)];
}
inline const char* name(Status s)
{
    static const char* n[] = {"SUCCESS", "LOC_PROT_ERR", "WR_FLUSH_ERR", "REM_ACCESS_ERR", "RNR_RETRY_EXC_ERR"};
    return n[static_cast<int>(s)];
}
inline const char* name(Op o)
{
    static const char* n[] = {"SEND", "RECV", "RDMA_WRITE"};
    return n[static_cast<int>(o)];
}

struct Mr                     // a registered memory region
{
    size_t offset;            // start inside the node's memory (the "virtual address")
    size_t length;
    unsigned access;
    uint32_t lkey;
    uint32_t rkey;
};

struct Sge                    // scatter/gather element: where the bytes are, and which key
{
    size_t addr;
    size_t length;
    uint32_t lkey;
};

struct Wr                     // a work request
{
    uint64_t id;
    Op op;
    Sge sge;
    size_t remoteAddr = 0;    // RDMA WRITE only
    uint32_t rkey = 0;        // RDMA WRITE only
};

struct Wc                     // a work completion
{
    uint64_t id;
    Op op;
    Status status;
    size_t bytes;
};

class Node
{
public:
    explicit Node(std::string n, size_t memBytes) : name(std::move(n)), memory(memBytes, 0) {}

    const Mr& regMr(size_t offset, size_t length, unsigned access)   // like ibv_reg_mr
    {
        const uint32_t key = nextKey;   // keys are unique across the whole model
        nextKey += 0x100;
        mrs.push_back(Mr{offset, length, access, key, key + 1});
        std::printf("[%s] reg_mr: addr %zu len %zu access 0x%x -> lkey 0x%x rkey 0x%x\n", name.c_str(),
                    offset, length, access, key, key + 1);
        return mrs.back();
    }
    const Mr* byLkey(uint32_t k) const
    {
        for (const Mr& m : mrs) {
            if (m.lkey == k) {
                return &m;
            }
        }
        return nullptr;
    }
    const Mr* byRkey(uint32_t k) const
    {
        for (const Mr& m : mrs) {
            if (m.rkey == k) {
                return &m;
            }
        }
        return nullptr;
    }
    bool pollCq(Wc& out)                                          // like ibv_poll_cq, one entry
    {
        if (cq.empty()) {
            return false;
        }
        out = cq.front();
        cq.pop_front();
        return true;
    }

    std::string name;
    std::vector<uint8_t> memory;
    std::deque<Wc> cq;

private:
    std::deque<Mr> mrs;   // deque: references returned by regMr stay valid
    static inline uint32_t nextKey = 0x1000;
};

inline bool inside(const Mr& m, size_t addr, size_t len)
{
    return addr >= m.offset && len <= m.length && addr - m.offset <= m.length - len;
}

class Qp
{
public:
    Qp(Node& owner, uint32_t num) : node(owner), qpn(num) {}

    bool modify(State to)                                          // like ibv_modify_qp
    {
        const bool ok = to == State::RESET || to == State::ERR || (state == State::RESET && to == State::INIT) ||
                        (state == State::INIT && to == State::RTR) || (state == State::RTR && to == State::RTS);
        std::printf("[%s] qp 0x%x modify %s -> %s: %s\n", node.name.c_str(), qpn, name(state), name(to),
                    ok ? "ok" : "rejected (EINVAL)");
        if (ok) {
            state = to;
            if (to == State::ERR) {
                flushAll();
            }
        }
        return ok;
    }
    void connect(Qp& p) { peer = &p; }

    int postSend(const Wr& wr)                                     // like ibv_post_send: 0 or an errno
    {
        if (state != State::RTS && state != State::ERR) {
            std::printf("[%s] post_send wr %llu in state %s: rejected (EINVAL)\n", node.name.c_str(),
                        static_cast<unsigned long long>(wr.id), name(state));
            return 22;
        }
        sq.push_back(wr);
        return 0;
    }
    int postRecv(const Wr& wr)                                     // like ibv_post_recv
    {
        if (state == State::RESET) {
            std::printf("[%s] post_recv in state RESET: rejected (EINVAL)\n", node.name.c_str());
            return 22;
        }
        rq.push_back(wr);
        return 0;
    }

    // Execute everything on the send queue ("the NIC works"). rnrRetry = how many times a SEND
    // that finds no posted receive at the target is retried before the error is reported.
    void progress(int rnrRetry = 3)
    {
        while (!sq.empty()) {
            const Wr wr = sq.front();
            sq.pop_front();
            if (state == State::ERR) {
                complete(node, wr, Status::WR_FLUSH_ERR, 0);
                continue;
            }
            const Mr* local = node.byLkey(wr.sge.lkey);
            if (local == nullptr || !inside(*local, wr.sge.addr, wr.sge.length)) {
                fault(wr, Status::LOC_PROT_ERR);
                continue;
            }
            if (wr.op == Op::RDMA_WRITE) {
                const Mr* target = peer->node.byRkey(wr.rkey);
                if (target == nullptr || (target->access & REMOTE_WRITE) == 0 ||
                    !inside(*target, wr.remoteAddr, wr.sge.length)) {
                    fault(wr, Status::REM_ACCESS_ERR);   // the responder refuses; the requester sees it
                    continue;
                }
                std::memcpy(&peer->node.memory[wr.remoteAddr], &node.memory[wr.sge.addr], wr.sge.length);
                complete(node, wr, Status::SUCCESS, wr.sge.length);   // no completion at the target
            } else {  // SEND
                int tries = 0;
                while (peer->rq.empty() && tries <= rnrRetry) {
                    std::printf("[%s] qp 0x%x: receiver not ready (no RECV posted at %s), attempt %d\n",
                                node.name.c_str(), qpn, peer->node.name.c_str(), tries + 1);
                    ++tries;
                }
                if (peer->rq.empty()) {
                    fault(wr, Status::RNR_RETRY_EXC_ERR);
                    continue;
                }
                const Wr rwr = peer->rq.front();
                peer->rq.pop_front();
                std::memcpy(&peer->node.memory[rwr.sge.addr], &node.memory[wr.sge.addr], wr.sge.length);
                complete(node, wr, Status::SUCCESS, wr.sge.length);
                complete(peer->node, rwr, Status::SUCCESS, wr.sge.length);   // two-sided: both see it
            }
        }
    }

    Node& node;
    uint32_t qpn;
    State state = State::RESET;

private:
    static void complete(Node& n, const Wr& wr, Status s, size_t bytes) { n.cq.push_back(Wc{wr.id, wr.op, s, bytes}); }
    void fault(const Wr& wr, Status s)
    {
        complete(node, wr, s, 0);
        std::printf("[%s] qp 0x%x: error %s on wr %llu -> state ERR\n", node.name.c_str(), qpn, name(s),
                    static_cast<unsigned long long>(wr.id));
        state = State::ERR;
        flushAll();
    }
    void flushAll()
    {
        while (!sq.empty()) {
            complete(node, sq.front(), Status::WR_FLUSH_ERR, 0);
            sq.pop_front();
        }
        while (!rq.empty()) {
            complete(node, rq.front(), Status::WR_FLUSH_ERR, 0);
            rq.pop_front();
        }
    }

    Qp* peer = nullptr;
    std::deque<Wr> sq;
    std::deque<Wr> rq;
};

inline void drain(Node& n)
{
    Wc wc{};
    while (n.pollCq(wc)) {
        std::printf("[%s] completion: wr %llu %s status %s bytes %zu\n", n.name.c_str(),
                    static_cast<unsigned long long>(wc.id), name(wc.op), name(wc.status), wc.bytes);
    }
}

}  // namespace toy
