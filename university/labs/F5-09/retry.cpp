// retry.cpp - what a timeout means, and what retrying does to a non-idempotent procedure.
// The procedure deposit(amount) adds to a balance. Its first execution is slow
// (300 ms, injected by slowFirstCall), longer than the client's 200 ms timeout.
//   A  give up    : no retry; the client cannot tell whether the deposit happened
//   B  retry      : retry with the same request id, server without duplicate detection
//   C  retry+dedup: retry with the same request id, server remembers replies by (client, id)
#include "rpc.hpp"

#include <cstdio>
#include <string>
#include <thread>

struct Account
{
    std::int64_t balance = 0;
};

void install(rpc::Server& s, Account& acct)
{
    s.add("deposit", [&acct](rpc::Reader& in, rpc::Writer& out) {
        acct.balance += in.i64();
        out.i64(acct.balance);
    });
    s.add("balance", [&acct](rpc::Reader&, rpc::Writer& out) { out.i64(acct.balance); });
}

std::string deposit100()
{
    rpc::Writer w;
    w.i64(100);
    return w.bytes();
}

std::int64_t value(const rpc::Reply& r)
{
    rpc::Reader in(r.body);
    return in.i64();
}

void scenario(const char* title, rpc::Dedup dedup, int attempts)
{
    std::printf("=== %s ===\n", title);
    Account acct;
    rpc::Server server(dedup);
    install(server, acct);
    server.slowFirstCall("deposit", 300);
    std::thread t([&server] { server.serve(1); });
    {
        rpc::Client c(server.port(), 7);
        const std::uint32_t id = c.newId();
        bool known = false;
        for (int attempt = 1; attempt <= attempts && !known; ++attempt) {
            auto r = c.call(id, "deposit", deposit100(), 200);
            if (r) {
                std::printf("client: deposit(100) id %u attempt %d -> reply, balance %lld\n", id,
                            attempt, static_cast<long long>(value(*r)));
                known = true;
            } else {
                std::printf("client: deposit(100) id %u attempt %d -> timeout after 200 ms\n", id,
                            attempt);
            }
        }
        if (!known) {
            std::printf("client: giving up; the deposit MAY OR MAY NOT have happened\n");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(400));   // let late replies arrive
        auto b = c.call(c.newId(), "balance", "", 2000);
        for (std::uint32_t s : c.takeStale()) {
            std::printf("client: dropped a late reply for id %u\n", s);
        }
        std::printf("client: balance() -> %lld   (one deposit of 100 was intended)\n",
                    b ? static_cast<long long>(value(*b)) : -1LL);
    }
    t.join();
    for (const std::string& line : server.journal()) {
        std::printf("server: %s\n", line.c_str());
    }
    std::printf("\n");
}

int main()
{
    scenario("A  give up after one timeout", rpc::Dedup::none, 1);
    scenario("B  retry, no duplicate detection (at-least-once)", rpc::Dedup::none, 3);
    scenario("C  retry, duplicate detection by (client, id)", rpc::Dedup::global, 3);
    return 0;
}
