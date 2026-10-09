// rdma_model.h - F1-53: the university's toy model of RDMA-style NICs (not a real API).
// Two hosts, each with memory, registered memory regions (with keys and permissions),
// a receive queue and a completion queue. The toy NIC moves bytes between the hosts'
// memories directly; the target host's CPU runs no code for one-sided operations.
// Names here are the toy's own; the real verbs API names are in the chapter's
// "not verified" box, to be checked in the rdma-core documentation.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace toy {

enum Access : unsigned { kLocalWrite = 1u, kRemoteWrite = 2u, kRemoteRead = 4u };
enum class Op { Write, Read, Send };

struct Region
{
    std::size_t start = 0;
    std::size_t length = 0;
    unsigned access = 0;
    std::uint32_t key = 0;     // one key here; real NICs give a local and a remote key
    bool valid = false;
};

struct Completion
{
    std::uint64_t id = 0;
    std::string what;
    std::string status;
};

struct Host
{
    explicit Host(std::string n) : name(std::move(n)), memory(256, '.') {}
    std::string name;
    std::vector<char> memory;
    std::vector<Region> regions;
    std::deque<std::size_t> receives;    // posted receive buffers (start offsets)
    std::vector<Completion> cq;

    std::uint32_t registerRegion(std::size_t start, std::size_t length, unsigned access)
    {
        Region r{start, length, access, nextKey_++, true};
        regions.push_back(r);
        return r.key;
    }
    void deregister(std::uint32_t key)
    {
        for (auto& r : regions) {
            if (r.key == key) {
                r.valid = false;
            }
        }
    }
    // Checks key, permission and bounds, exactly what a remote request must pass.
    bool allowed(std::uint32_t key, std::size_t at, std::size_t len, unsigned need) const
    {
        for (const auto& r : regions) {
            if (r.valid && r.key == key && (r.access & need) == need && at >= r.start &&
                at + len <= r.start + r.length) {
                return true;
            }
        }
        return false;
    }
    std::string text(std::size_t at, std::size_t len) const
    {
        return std::string(memory.begin() + static_cast<long>(at), memory.begin() + static_cast<long>(at + len));
    }

private:
    std::uint32_t nextKey_ = 0x1000u + static_cast<std::uint32_t>(name.size()) * 0x100u;
};

// One work request posted on `from`'s send queue towards `to`; the toy NIC executes it.
inline void post(Host& from, Host& to, std::uint64_t id, Op op, std::size_t local,
                 std::size_t len, std::size_t remote, std::uint32_t rkey)
{
    const char* what = op == Op::Write ? "RDMA WRITE" : (op == Op::Read ? "RDMA READ" : "SEND");
    std::string status = "success";
    if (op == Op::Send) {
        if (to.receives.empty()) {
            status = "receiver not ready (no receive posted)";
        } else {
            const std::size_t dst = to.receives.front();
            to.receives.pop_front();
            std::memcpy(&to.memory[dst], &from.memory[local], len);
            to.cq.push_back({id, "RECEIVE", "success"});   // two-sided: the target sees it
        }
    } else {
        const unsigned need = op == Op::Write ? kRemoteWrite : kRemoteRead;
        if (!to.allowed(rkey, remote, len, need)) {
            status = "remote access error";
        } else if (op == Op::Write) {
            std::memcpy(&to.memory[remote], &from.memory[local], len);
        } else {
            std::memcpy(&from.memory[local], &to.memory[remote], len);
        }
    }
    from.cq.push_back({id, what, status});
    std::printf("  %s posts %-10s id %llu (%zu bytes, key 0x%x) -> %s\n", from.name.c_str(), what,
                static_cast<unsigned long long>(id), len, rkey, status.c_str());
}

inline void drain(Host& h)
{
    if (h.cq.empty()) {
        std::printf("  %s completion queue: empty\n", h.name.c_str());
    }
    for (const auto& c : h.cq) {
        std::printf("  %s completion: id %llu %s %s\n", h.name.c_str(),
                    static_cast<unsigned long long>(c.id), c.what.c_str(), c.status.c_str());
    }
    h.cq.clear();
}

}  // namespace toy
