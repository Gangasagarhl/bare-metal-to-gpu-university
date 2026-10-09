// rpc_demo.cpp - the RPC library of rpc.hpp in use: a server with three procedures,
// client stubs that make remote calls look like local functions, and the bytes on the wire.
#include "rpc.hpp"

#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>

// ---- client stubs: marshal the arguments, call, unmarshal the result
std::int64_t add(rpc::Client& c, std::int64_t a, std::int64_t b)
{
    rpc::Writer args;
    args.i64(a);
    args.i64(b);
    auto r = c.call(c.newId(), "add", args.bytes(), 2000);
    if (!r) {
        throw std::runtime_error("add: no reply within 2000 ms (outcome unknown)");
    }
    if (!r->ok) {
        throw std::runtime_error("add: remote error: " + r->body);
    }
    rpc::Reader res(r->body);
    return res.i64();
}

std::string upper(rpc::Client& c, const std::string& s)
{
    rpc::Writer args;
    args.str(s);
    auto r = c.call(c.newId(), "upper", args.bytes(), 2000);
    if (!r || !r->ok) {
        throw std::runtime_error("upper failed");
    }
    rpc::Reader res(r->body);
    return res.str();
}

std::string hex(const std::string& bytes)
{
    std::string out;
    char buf[4];
    for (unsigned char b : bytes) {
        std::snprintf(buf, sizeof buf, "%02x ", b);
        out += buf;
    }
    return out;
}

int main()
{
    rpc::Server server;
    server.add("add", [](rpc::Reader& in, rpc::Writer& out) {
        const std::int64_t a = in.i64();
        const std::int64_t b = in.i64();
        out.i64(a + b);
    });
    server.add("upper", [](rpc::Reader& in, rpc::Writer& out) {
        std::string s = in.str();
        for (char& ch : s) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        out.str(s);
    });
    server.add("checksum", [](rpc::Reader& in, rpc::Writer& out) {
        const std::string s = in.str();
        std::uint32_t sum = 2166136261u;            // 32-bit FNV-1a hash of the bytes
        for (unsigned char ch : s) {
            sum = (sum ^ ch) * 16777619u;
        }
        out.u32(static_cast<std::uint32_t>(s.size()));
        out.u32(sum);
    });

    std::thread serverThread([&server] { server.serve(1); });
    {
        rpc::Client client(server.port(), 1);

        rpc::Writer args;
        args.i64(2);
        args.i64(40);
        const std::string req = rpc::Client::request(1, 1, "add", args.bytes());
        std::printf("request add(2, 40) as bytes (%zu bytes, before the 4-byte frame length):\n"
                    "  %s\n", req.size(), hex(req).c_str());

        std::printf("add(2, 40)          = %lld\n", static_cast<long long>(add(client, 2, 40)));
        std::printf("add(-5, 3)          = %lld\n", static_cast<long long>(add(client, -5, 3)));
        std::printf("upper(\"hello rpc\") = \"%s\"\n", upper(client, "hello rpc").c_str());

        // a procedure the server does not have: the error travels back as a reply
        auto r = client.call(client.newId(), "multiply", args.bytes(), 2000);
        std::printf("multiply(...)       -> %s: %s\n", r && r->ok ? "ok" : "error",
                    r ? r->body.c_str() : "(no reply)");

        // a large argument: TCP delivers it in pieces; the frame length puts it back together
        const std::string big(1 << 20, 'x');
        rpc::Writer bigArgs;
        bigArgs.str(big);
        auto rb = client.call(client.newId(), "checksum", bigArgs.bytes(), 5000);
        if (rb && rb->ok) {
            rpc::Reader res(rb->body);
            const std::uint32_t n = res.u32();
            const std::uint32_t sum = res.u32();
            std::uint32_t local = 2166136261u;
            for (unsigned char ch : big) {
                local = (local ^ ch) * 16777619u;
            }
            std::printf("checksum(1 MiB)     = length %u, sum %08x (computed locally: %08x) %s\n",
                        n, sum, local, sum == local ? "MATCH" : "MISMATCH");
        }
    }   // the client's destructor closes the connection; the server's loop then ends
    serverThread.join();
    std::printf("\nserver journal:\n");
    for (const std::string& line : server.journal()) {
        std::printf("  %s\n", line.c_str());
    }
    return 0;
}
