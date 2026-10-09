// reconnect.cpp - generates the evidence pack of the F5-09 forensic lab "the double deposit".
// The server has duplicate detection switched on, yet a retried deposit runs twice.
// (Why is explained only in the answer key.) The client's retry policy is a common one:
// after a timeout, drop the connection, open a fresh one and send the request again.
#include "rpc.hpp"

#include <cstdio>
#include <string>
#include <thread>

int main()
{
    std::int64_t balance = 0;
    rpc::Server server(rpc::Dedup::perConnection);
    server.add("deposit", [&balance](rpc::Reader& in, rpc::Writer& out) {
        balance += in.i64();
        out.i64(balance);
    });
    server.add("balance", [&balance](rpc::Reader&, rpc::Writer& out) { out.i64(balance); });
    server.slowFirstCall("deposit", 300);
    std::printf("=== server start-up ===\n");
    std::printf("config: duplicate detection = enabled (reply cache keyed by client and id)\n");
    std::printf("config: procedures = deposit, balance\n\n");

    std::thread t([&server] { server.serve(2); });
    std::printf("=== client.log (client 7, timeout 200 ms, up to 3 attempts) ===\n");
    {
        rpc::Client c(server.port(), 7);
        rpc::Writer args;
        args.i64(100);
        const std::uint32_t id = c.newId();
        for (int attempt = 1; attempt <= 3; ++attempt) {
            auto r = c.call(id, "deposit", args.bytes(), 200);
            if (r) {
                rpc::Reader in(r->body);
                std::printf("deposit id=%u attempt=%d: ok, new balance %lld\n", id, attempt,
                            static_cast<long long>(in.i64()));
                break;
            }
            std::printf("deposit id=%u attempt=%d: timeout; closing connection, reconnecting\n",
                        id, attempt);
            c.close();
            c.reconnect();
        }
        auto b = c.call(c.newId(), "balance", "", 2000);
        if (b) {
            rpc::Reader in(b->body);
            std::printf("balance id=2: ok, %lld\n", static_cast<long long>(in.i64()));
        }
    }
    t.join();
    std::printf("\n=== server.log ===\n");
    for (const std::string& line : server.journal()) {
        std::printf("%s\n", line.c_str());
    }
    return 0;
}
