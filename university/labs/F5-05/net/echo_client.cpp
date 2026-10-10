// echo_client.cpp - send N bytes to an echo server and check that exactly the same bytes
// come back, using a checksum (64-bit FNV-1a) of what was sent and of what was received.
// A second thread sends while the main thread receives: if one thread first sent everything
// and only then read, both sides' buffers could fill up and the two programs would wait
// for each other forever.
// Usage: echo_client <host> <port> <bytes>
#include "socket.hpp"

#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <thread>
#include <vector>

std::uint64_t fnv1a(std::uint64_t h, const char* data, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<unsigned char>(data[i]);
        h *= 1099511628211ULL;
    }
    return h;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s <host> <port> <bytes>\n", argv[0]);
        return 2;
    }
    try {
        const std::size_t total = std::stoull(argv[3]);
        const Socket s = connectTo(argv[1], argv[2]);
        constexpr std::uint64_t seed = 14695981039346656037ULL;
        std::uint64_t sentSum = seed;
        const auto start = std::chrono::steady_clock::now();

        std::exception_ptr senderError;               // an exception must not escape a thread
        std::jthread sender([&] {
            try {
                std::vector<char> block(64 * 1024);
                std::uint32_t x = 12345;                  // deterministic pseudo-random bytes
                for (std::size_t done = 0; done < total;) {
                    const std::size_t n = std::min(block.size(), total - done);
                    for (std::size_t i = 0; i < n; ++i) {
                        x = x * 1103515245u + 12345u;
                        block[i] = static_cast<char>(x >> 24);
                    }
                    sentSum = fnv1a(sentSum, block.data(), n);
                    sendAll(s, block.data(), n);
                    done += n;
                }
                ::shutdown(s.fd(), SHUT_WR);              // tell the server: no more data from us
            } catch (...) {
                senderError = std::current_exception();
            }
        });

        std::uint64_t gotSum = seed;
        std::size_t got = 0;
        std::vector<char> buf(64 * 1024);
        for (;;) {
            const ssize_t n = ::recv(s.fd(), buf.data(), buf.size(), 0);
            if (n < 0) { throwErrno("recv"); }
            if (n == 0) { break; }                    // the server closed after echoing all
            gotSum = fnv1a(gotSum, buf.data(), static_cast<std::size_t>(n));
            got += static_cast<std::size_t>(n);
        }
        sender.join();                                 // wait for the sending thread
        if (senderError) { std::rethrow_exception(senderError); }
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const double sec = std::chrono::duration<double>(elapsed).count();
        const bool pass = got == total && gotSum == sentSum;
        std::printf("sent %zu bytes, received %zu bytes in %.3f s\n", total, got, sec);
        std::printf("checksum sent %016llx received %016llx -> %s\n",
                    static_cast<unsigned long long>(sentSum),
                    static_cast<unsigned long long>(gotSum), pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "echo_client: %s\n", e.what());
        return 1;
    }
}
