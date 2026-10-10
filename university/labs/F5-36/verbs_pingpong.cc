// verbs_pingpong.cc - DS401 F5-36, Listing 1: a ping-pong over InfiniBand verbs (libibverbs).
// One reliable connection (RC) queue pair per side; SEND/RECV of a small message, N times.
// The queue-pair numbers, PSNs, LIDs and GIDs are exchanged "out of band" over TCP.
//   server:  verbs_pingpong
//   client:  verbs_pingpong <server-ipv4-address> [gid-index]
// Build:  g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror verbs_pingpong.cc -o verbs_pingpong -libverbs
// Needs an RDMA device (a NIC, or Soft-RoCE); the build container has none (see the chapter).
#include <infiniband/verbs.h>

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr uint16_t kTcpPort = 18601;   // our choice for the out-of-band exchange
constexpr uint8_t kPort = 1;           // first port of the device
constexpr size_t kMsgSize = 64;
constexpr int kIters = 1000;

[[noreturn]] void fail(const std::string& what)
{
    throw std::runtime_error(what + ": " + std::strerror(errno));
}

// RAII owners for the verbs objects (destroyed in reverse order of creation).
template <typename T, int (*Destroy)(T*)>
struct Deleter
{
    void operator()(T* p) const { Destroy(p); }
};
using DeviceList = std::unique_ptr<ibv_device*, decltype([](ibv_device** l) { ibv_free_device_list(l); })>;
using Context = std::unique_ptr<ibv_context, Deleter<ibv_context, ibv_close_device>>;
using Pd = std::unique_ptr<ibv_pd, Deleter<ibv_pd, ibv_dealloc_pd>>;
using Mr = std::unique_ptr<ibv_mr, Deleter<ibv_mr, ibv_dereg_mr>>;
using Cq = std::unique_ptr<ibv_cq, Deleter<ibv_cq, ibv_destroy_cq>>;
using Qp = std::unique_ptr<ibv_qp, Deleter<ibv_qp, ibv_destroy_qp>>;

struct Endpoint            // what each side tells the other out of band
{
    uint16_t lid;          // InfiniBand local identifier (0 on RoCE)
    uint32_t qpn;          // queue pair number
    uint32_t psn;          // first packet sequence number
    ibv_gid gid;           // global identifier (the IP-based address on RoCE)
};

class Tcp                  // the out-of-band channel: one TCP connection
{
public:
    explicit Tcp(const char* serverAddr)
    {
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons(kTcpPort);
        if (serverAddr == nullptr) {
            const int l = ::socket(AF_INET, SOCK_STREAM, 0);
            const int one = 1;
            ::setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
            a.sin_addr.s_addr = htonl(INADDR_ANY);
            if (l < 0 || ::bind(l, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0 || ::listen(l, 1) != 0) {
                fail("tcp listen");
            }
            fd_ = ::accept(l, nullptr, nullptr);
            ::close(l);
        } else {
            fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (::inet_pton(AF_INET, serverAddr, &a.sin_addr) != 1) {
                throw std::runtime_error("not an IPv4 address");
            }
            if (::connect(fd_, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
                fail("tcp connect");
            }
        }
        if (fd_ < 0) {
            fail("tcp");
        }
    }
    ~Tcp() { ::close(fd_); }
    Tcp(const Tcp&) = delete;
    Tcp& operator=(const Tcp&) = delete;
    Endpoint exchange(const Endpoint& mine)        // both sides send first, then receive
    {
        Endpoint theirs{};
        if (::send(fd_, &mine, sizeof mine, 0) != static_cast<ssize_t>(sizeof mine) ||
            ::recv(fd_, &theirs, sizeof theirs, MSG_WAITALL) != static_cast<ssize_t>(sizeof theirs)) {
            fail("endpoint exchange");
        }
        return theirs;      // same program on both sides, same struct layout
    }
    void barrier()
    {
        char c = 'b';
        if (::send(fd_, &c, 1, 0) != 1 || ::recv(fd_, &c, 1, MSG_WAITALL) != 1) {
            fail("barrier");
        }
    }

private:
    int fd_ = -1;
};

void modify(ibv_qp* qp, ibv_qp_attr& attr, int mask, const char* step)
{
    const int rc = ibv_modify_qp(qp, &attr, mask);
    if (rc != 0) {
        errno = rc;
        fail(std::string("ibv_modify_qp to ") + step);
    }
}

void toInit(ibv_qp* qp)
{
    ibv_qp_attr a{};
    a.qp_state = IBV_QPS_INIT;
    a.pkey_index = 0;
    a.port_num = kPort;
    a.qp_access_flags = 0;   // SEND/RECV only: no remote access to our memory
    modify(qp, a, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS, "INIT");
}

void toRtr(ibv_qp* qp, const Endpoint& remote, ibv_mtu mtu, int gidIndex)
{
    ibv_qp_attr a{};
    a.qp_state = IBV_QPS_RTR;
    a.path_mtu = mtu;
    a.dest_qp_num = remote.qpn;
    a.rq_psn = remote.psn;
    a.max_dest_rd_atomic = 1;
    a.min_rnr_timer = 12;
    a.ah_attr.dlid = remote.lid;
    a.ah_attr.sl = 0;
    a.ah_attr.port_num = kPort;
    if (remote.gid.global.interface_id != 0 || remote.gid.global.subnet_prefix != 0) {
        a.ah_attr.is_global = 1;           // RoCE (and routed InfiniBand) need a GRH
        a.ah_attr.grh.dgid = remote.gid;
        a.ah_attr.grh.sgid_index = static_cast<uint8_t>(gidIndex);
        a.ah_attr.grh.hop_limit = 1;
    }
    modify(qp, a, IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                      IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER, "RTR");
}

void toRts(ibv_qp* qp, uint32_t myPsn)
{
    ibv_qp_attr a{};
    a.qp_state = IBV_QPS_RTS;
    a.sq_psn = myPsn;
    a.timeout = 14;
    a.retry_cnt = 7;
    a.rnr_retry = 7;
    a.max_rd_atomic = 1;
    modify(qp, a, IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
                      IBV_QP_MAX_QP_RD_ATOMIC, "RTS");
}

void postRecv(ibv_qp* qp, ibv_mr* mr, char* buf)
{
    ibv_sge sge{reinterpret_cast<uintptr_t>(buf), static_cast<uint32_t>(kMsgSize), mr->lkey};
    ibv_recv_wr wr{};
    wr.wr_id = 1;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    ibv_recv_wr* bad = nullptr;
    if (const int rc = ibv_post_recv(qp, &wr, &bad); rc != 0) {
        errno = rc;
        fail("ibv_post_recv");
    }
}

void postSend(ibv_qp* qp, ibv_mr* mr, char* buf)
{
    ibv_sge sge{reinterpret_cast<uintptr_t>(buf), static_cast<uint32_t>(kMsgSize), mr->lkey};
    ibv_send_wr wr{};
    wr.wr_id = 2;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_SEND;
    wr.send_flags = IBV_SEND_SIGNALED;     // ask for a completion when the send is done
    ibv_send_wr* bad = nullptr;
    if (const int rc = ibv_post_send(qp, &wr, &bad); rc != 0) {
        errno = rc;
        fail("ibv_post_send");
    }
}

void waitOne(ibv_cq* cq)                   // busy-poll until one completion arrives
{
    ibv_wc wc{};
    int n = 0;
    while ((n = ibv_poll_cq(cq, 1, &wc)) == 0) {
    }
    if (n < 0) {
        fail("ibv_poll_cq");
    }
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error(std::string("work completion error: ") + ibv_wc_status_str(wc.status));
    }
}

int run(const char* serverAddr, int gidIndex)
{
    int num = 0;
    errno = 0;
    DeviceList list(ibv_get_device_list(&num));
    if (!list || num == 0) {
        std::printf("no RDMA device found: ibv_get_device_list returned %d devices (errno %d: %s)\n", num,
                    errno, std::strerror(errno));
        std::printf("nothing to test on this machine; exit code 2\n");
        return 2;
    }
    Context ctx(ibv_open_device(list.get()[0]));
    if (!ctx) {
        fail("ibv_open_device");
    }
    std::printf("device %s\n", ibv_get_device_name(list.get()[0]));
    ibv_port_attr port{};
    if (ibv_query_port(ctx.get(), kPort, &port) != 0) {
        fail("ibv_query_port");
    }
    Endpoint me{};
    me.lid = port.lid;
    if (ibv_query_gid(ctx.get(), kPort, gidIndex, &me.gid) != 0) {
        fail("ibv_query_gid");
    }
    Pd pd(ibv_alloc_pd(ctx.get()));
    if (!pd) {
        fail("ibv_alloc_pd");
    }
    std::vector<char> buf(2 * kMsgSize);   // [0, size) to send, [size, 2*size) to receive
    Mr mr(ibv_reg_mr(pd.get(), buf.data(), buf.size(), IBV_ACCESS_LOCAL_WRITE));
    if (!mr) {
        fail("ibv_reg_mr");
    }
    Cq sendCq(ibv_create_cq(ctx.get(), 16, nullptr, nullptr, 0));   // one CQ per queue, so a
    Cq recvCq(ibv_create_cq(ctx.get(), 16, nullptr, nullptr, 0));   // completion is never mistaken
    if (!sendCq || !recvCq) {
        fail("ibv_create_cq");
    }
    ibv_qp_init_attr init{};
    init.send_cq = sendCq.get();
    init.recv_cq = recvCq.get();
    init.cap.max_send_wr = 1;
    init.cap.max_recv_wr = 1;
    init.cap.max_send_sge = 1;
    init.cap.max_recv_sge = 1;
    init.qp_type = IBV_QPT_RC;
    Qp qp(ibv_create_qp(pd.get(), &init));
    if (!qp) {
        fail("ibv_create_qp");
    }
    me.qpn = qp->qp_num;
    me.psn = static_cast<uint32_t>(std::rand()) & 0xffffff;   // PSNs are 24 bits
    toInit(qp.get());

    Tcp oob(serverAddr);
    const Endpoint peer = oob.exchange(me);
    std::printf("local QPN 0x%06x PSN 0x%06x, remote QPN 0x%06x PSN 0x%06x\n", me.qpn, me.psn, peer.qpn, peer.psn);
    toRtr(qp.get(), peer, port.active_mtu, gidIndex);
    toRts(qp.get(), me.psn);

    char* sendBuf = buf.data();
    char* recvBuf = buf.data() + kMsgSize;
    std::vector<double> us;
    postRecv(qp.get(), mr.get(), recvBuf);
    oob.barrier();                                   // both sides have a receive posted
    const bool client = serverAddr != nullptr;
    for (int i = 0; i < kIters; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        if (client) {
            std::snprintf(sendBuf, kMsgSize, "ping %d", i);
            postSend(qp.get(), mr.get(), sendBuf);
            waitOne(sendCq.get());                   // our ping was acknowledged
            waitOne(recvCq.get());                   // the pong arrived
            postRecv(qp.get(), mr.get(), recvBuf);   // ready for the next one
        } else {
            waitOne(recvCq.get());                   // a ping arrived
            std::memcpy(sendBuf, recvBuf, kMsgSize);
            sendBuf[1] = 'o';                        // "ping" -> "pong"
            postRecv(qp.get(), mr.get(), recvBuf);   // the buffer is free again
            postSend(qp.get(), mr.get(), sendBuf);
            waitOne(sendCq.get());
        }
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    std::sort(us.begin(), us.end());
    std::printf("%d round trips of %zu bytes: median %.2f us (last message: \"%s\")\n", kIters, kMsgSize,
                us[us.size() / 2], recvBuf);
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        return run(argc > 1 ? argv[1] : nullptr, argc > 2 ? std::atoi(argv[2]) : 0);
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
}
