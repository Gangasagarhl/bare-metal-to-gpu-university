// rpc.hpp - a small remote procedure call library over TCP (DS301, chapter F5-09).
// Wire format, all integers big-endian ("network byte order"):
//   frame   = u32 length, then `length` bytes of payload    (TCP is a byte stream: F5-04)
//   request = u32 client, u32 id, str method, arguments      (arguments written by a stub)
//   reply   = u32 id, u8 status (0 ok, 1 error), result or error text
//   str     = u32 length, then the bytes
// The server serves its connections one after another (enough for these labs) and can
// remember replies so that a retried request is not executed twice (Dedup below).
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rpc {

// ---------------------------------------------------------------- a file descriptor (RAII)
class Fd
{
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    Fd(Fd&& o) noexcept : fd_(std::exchange(o.fd_, -1)) {}
    Fd& operator=(Fd&& o) noexcept
    {
        if (this != &o) {
            reset();
            fd_ = std::exchange(o.fd_, -1);
        }
        return *this;
    }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    ~Fd() { reset(); }
    int get() const { return fd_; }
    void reset()
    {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_;
};

[[noreturn]] inline void fail(const std::string& what)
{
    throw std::runtime_error(what + ": " + std::strerror(errno));
}

// ---------------------------------------------------------------- marshalling
class Writer
{
public:
    void u8(std::uint8_t v) { buf_.push_back(static_cast<char>(v)); }
    void u32(std::uint32_t v)
    {
        for (int shift = 24; shift >= 0; shift -= 8) {
            u8(static_cast<std::uint8_t>(v >> shift));
        }
    }
    void i64(std::int64_t v)
    {
        const auto u = static_cast<std::uint64_t>(v);   // two's complement bits
        u32(static_cast<std::uint32_t>(u >> 32));
        u32(static_cast<std::uint32_t>(u));
    }
    void str(const std::string& s)
    {
        u32(static_cast<std::uint32_t>(s.size()));
        buf_ += s;
    }
    void raw(const std::string& s) { buf_ += s; }
    const std::string& bytes() const { return buf_; }

private:
    std::string buf_;
};

class Reader
{
public:
    explicit Reader(const std::string& b) : b_(b) {}
    std::uint8_t u8()
    {
        need(1);
        return static_cast<std::uint8_t>(b_[pos_++]);
    }
    std::uint32_t u32()
    {
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8) | u8();
        }
        return v;
    }
    std::int64_t i64()
    {
        const std::uint64_t hi = u32();
        const std::uint64_t lo = u32();
        return static_cast<std::int64_t>((hi << 32) | lo);
    }
    std::string str()
    {
        const std::uint32_t n = u32();
        need(n);
        std::string s = b_.substr(pos_, n);
        pos_ += n;
        return s;
    }
    std::string rest()
    {
        std::string s = b_.substr(pos_);
        pos_ = b_.size();
        return s;
    }

private:
    void need(std::size_t n) const
    {
        if (b_.size() - pos_ < n) {
            throw std::runtime_error("malformed message: too few bytes");
        }
    }
    const std::string& b_;
    std::size_t pos_ = 0;
};

// ---------------------------------------------------------------- framing over a byte stream
inline void sendAll(int fd, const std::string& data)
{
    std::size_t done = 0;
    while (done < data.size()) {
        // MSG_NOSIGNAL: a peer that has gone away gives an error, not a SIGPIPE signal
        const ssize_t n = ::send(fd, data.data() + done, data.size() - done, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            fail("send");
        }
        done += static_cast<std::size_t>(n);
    }
}

// Reads exactly n bytes. Returns false if the peer closed the connection before the first byte.
inline bool recvAll(int fd, char* p, std::size_t n)
{
    std::size_t done = 0;
    while (done < n) {
        const ssize_t got = ::recv(fd, p + done, n - done, 0);
        if (got == 0) {
            if (done == 0) {
                return false;
            }
            throw std::runtime_error("connection closed in the middle of a frame");
        }
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            fail("recv");
        }
        done += static_cast<std::size_t>(got);
    }
    return true;
}

inline void writeFrame(int fd, const std::string& payload)
{
    Writer w;
    w.u32(static_cast<std::uint32_t>(payload.size()));
    w.raw(payload);
    sendAll(fd, w.bytes());
}

inline std::optional<std::string> readFrame(int fd)
{
    char len[4];
    if (!recvAll(fd, len, 4)) {
        return std::nullopt;
    }
    const std::string lenBytes(len, 4);
    const std::uint32_t n = Reader(lenBytes).u32();
    if (n > (64u << 20)) {
        throw std::runtime_error("frame too large");
    }
    std::string payload(n, '\0');
    if (n > 0 && !recvAll(fd, payload.data(), n)) {
        throw std::runtime_error("connection closed in the middle of a frame");
    }
    return payload;
}

// true if fd has data (or end of stream) within timeoutMs
inline bool waitReadable(int fd, int timeoutMs)
{
    pollfd p{fd, POLLIN, 0};
    for (;;) {
        const int r = ::poll(&p, 1, timeoutMs);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r < 0) {
            fail("poll");
        }
        return r > 0;
    }
}

// ---------------------------------------------------------------- server
enum class Dedup { none, perConnection, global };

struct Reply
{
    std::uint32_t id = 0;
    bool ok = false;
    std::string body;   // result bytes, or the error text
};

class Server
{
public:
    using Handler = std::function<void(Reader& args, Writer& result)>;

    explicit Server(Dedup dedup = Dedup::none) : dedup_(dedup)
    {
        listener_ = Fd(::socket(AF_INET, SOCK_STREAM, 0));
        if (listener_.get() < 0) {
            fail("socket");
        }
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = 0;                                // 0: the kernel picks a free port
        if (::bind(listener_.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) < 0) {
            fail("bind");
        }
        if (::listen(listener_.get(), 8) < 0) {
            fail("listen");
        }
        socklen_t len = sizeof a;
        if (::getsockname(listener_.get(), reinterpret_cast<sockaddr*>(&a), &len) < 0) {
            fail("getsockname");
        }
        port_ = ntohs(a.sin_port);
    }

    std::uint16_t port() const { return port_; }
    void add(const std::string& method, Handler h) { handlers_[method] = std::move(h); }

    // Fault injection: the first execution of `method` takes `ms` milliseconds longer.
    void slowFirstCall(const std::string& method, int ms)
    {
        slowMethod_ = method;
        slowMs_ = ms;
    }

    // Accepts `connections` connections, one after another, and serves each until it closes.
    void serve(int connections)
    {
        for (int c = 1; c <= connections; ++c) {
            Fd conn(::accept(listener_.get(), nullptr, nullptr));
            if (conn.get() < 0) {
                fail("accept");
            }
            if (dedup_ == Dedup::perConnection) {
                saved_.clear();                        // this mode remembers replies per connection
            }
            log("connection " + std::to_string(c) + " opened");
            try {
                while (auto frame = readFrame(conn.get())) {
                    writeFrame(conn.get(), handle(*frame));
                }
            } catch (const std::exception&) {
                // a peer that vanished mid-reply ends this connection, not the server
            }
            log("connection " + std::to_string(c) + " ended");
        }
    }

    const std::vector<std::string>& journal() const { return journal_; }

private:
    void log(const std::string& line) { journal_.push_back(line); }

    std::string handle(const std::string& frame)
    {
        Reader in(frame);
        const std::uint32_t client = in.u32();
        const std::uint32_t id = in.u32();
        const std::string method = in.str();
        const std::uint64_t key = (std::uint64_t{client} << 32) | id;
        const std::string tag = "request " + std::to_string(client) + "/" + std::to_string(id)
                                + " " + method;
        if (dedup_ != Dedup::none) {
            if (auto it = saved_.find(key); it != saved_.end()) {
                log(tag + ": duplicate, saved reply sent again, NOT executed");
                return it->second;
            }
        }
        Writer out;
        out.u32(id);
        auto h = handlers_.find(method);
        if (h == handlers_.end()) {
            out.u8(1);
            out.raw("no such method: " + method);
            log(tag + ": rejected (unknown method)");
        } else {
            Writer result;
            try {
                h->second(in, result);
                out.u8(0);
                out.raw(result.bytes());
                log(tag + ": executed");
            } catch (const std::exception& e) {
                out.u8(1);
                out.raw(e.what());
                log(tag + ": failed: " + e.what());
            }
            if (method == slowMethod_ && slowMs_ > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(slowMs_));
                slowMs_ = 0;
            }
        }
        if (dedup_ != Dedup::none) {
            saved_[key] = out.bytes();
        }
        return out.bytes();
    }

    Dedup dedup_;
    Fd listener_;
    std::uint16_t port_ = 0;
    std::map<std::string, Handler> handlers_;
    std::map<std::uint64_t, std::string> saved_;   // (client, id) -> reply already sent
    std::vector<std::string> journal_;
    std::string slowMethod_;
    int slowMs_ = 0;
};

// ---------------------------------------------------------------- client
class Client
{
public:
    Client(std::uint16_t port, std::uint32_t clientId) : port_(port), client_(clientId)
    {
        reconnect();
    }

    void reconnect()
    {
        conn_ = Fd(::socket(AF_INET, SOCK_STREAM, 0));
        if (conn_.get() < 0) {
            fail("socket");
        }
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(port_);
        if (::connect(conn_.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) < 0) {
            fail("connect");
        }
    }

    void close() { conn_.reset(); }

    std::uint32_t newId() { return ++lastId_; }

    static std::string request(std::uint32_t client, std::uint32_t id, const std::string& method,
                               const std::string& args)
    {
        Writer w;
        w.u32(client);
        w.u32(id);
        w.str(method);
        w.raw(args);
        return w.bytes();
    }

    // Sends one request and waits up to timeoutMs for the reply with the same id.
    // Replies with other ids (late answers to earlier, abandoned requests) are dropped.
    // Returns nothing on timeout: the caller does NOT know whether the server executed it.
    std::optional<Reply> call(std::uint32_t id, const std::string& method,
                              const std::string& args, int timeoutMs)
    {
        writeFrame(conn_.get(), request(client_, id, method, args));
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(timeoutMs);
        for (;;) {
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
            if (left <= 0 || !waitReadable(conn_.get(), static_cast<int>(left))) {
                return std::nullopt;
            }
            auto frame = readFrame(conn_.get());
            if (!frame) {
                throw std::runtime_error("server closed the connection");
            }
            Reader in(*frame);
            Reply r;
            r.id = in.u32();
            r.ok = in.u8() == 0;
            r.body = in.rest();
            if (r.id == id) {
                return r;
            }
            stale_.push_back(r.id);
        }
    }

    std::vector<std::uint32_t> takeStale() { return std::exchange(stale_, {}); }

private:
    std::uint16_t port_;
    std::uint32_t client_;
    std::uint32_t lastId_ = 0;
    Fd conn_;
    std::vector<std::uint32_t> stale_;
};

}  // namespace rpc
