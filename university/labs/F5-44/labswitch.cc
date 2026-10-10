// labswitch.cc - DS403 host program: the cluster's Ethernet switch.
// Each QEMU node's NIC is attached with "-netdev dgram": the node sends every Ethernet
// frame as one UDP datagram to this program, which forwards it to the other nodes.
// The switch can cut the network in two for a time window (a partition) and logs what it
// forwards and drops. Usage:
//   labswitch <nodes> <base-port> <run-ms> [<cut-from-ms> <cut-to-ms> <groupA> <groupB>]
// Node i sends to 127.0.0.1:(base+10+i) and receives on 127.0.0.1:(base+i).
// It stops after <run-ms> or at SIGTERM (sent by lablib.sh when every node has halted).
// Example partition: 5000 15000 1 23  (node 1 alone, nodes 2 and 3 together).
#include <arpa/inet.h>
#include <csignal>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void on_term(int) { stop_requested = 1; }

const char* type_name(int t)
{
    switch (t) {
    case 1: return "HEARTBEAT";
    case 2: return "PS_REQ";
    case 3: return "PS_REP";
    case 4: return "RUN_REQ";
    case 5: return "RUN_REP";
    case 10: return "DFS_T";
    case 11: return "DFS_R";
    case 20: return "VOTE_REQ";
    case 21: return "VOTE_REP";
    case 22: return "APPEND";
    case 23: return "APPEND_REP";
    case 24: return "CLIENT";
    case 25: return "CLIENT_REP";
    case 26: return "JOB_DONE";
    default: return "OTHER";
    }
}

int group_of(int node, const std::string& a)
{
    return a.find(static_cast<char>('0' + node)) != std::string::npos ? 0 : 1;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc != 4 && argc != 8) {
        std::fprintf(stderr, "usage: labswitch <nodes> <base-port> <run-ms> [<from-ms> <to-ms> <groupA> <groupB>]\n");
        return 2;
    }
    const int nodes = std::atoi(argv[1]);
    const int base = std::atoi(argv[2]);
    const long run_ms = std::atol(argv[3]);
    const bool cut = argc == 8;
    const long cut_from = cut ? std::atol(argv[4]) : 0, cut_to = cut ? std::atol(argv[5]) : 0;
    const std::string group_a = cut ? argv[6] : "", group_b = cut ? argv[7] : "";
    if (nodes < 1 || nodes > 8) return 2;

    std::vector<int> fds(static_cast<size_t>(nodes) + 1, -1);
    std::vector<sockaddr_in> peer(static_cast<size_t>(nodes) + 1);
    for (int i = 1; i <= nodes; ++i) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in me{};
        me.sin_family = AF_INET;
        me.sin_port = htons(static_cast<uint16_t>(base + 10 + i));
        me.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (fd < 0 || bind(fd, reinterpret_cast<sockaddr*>(&me), sizeof me) != 0) {
            std::perror("bind");
            return 1;
        }
        fds[static_cast<size_t>(i)] = fd;
        sockaddr_in& p = peer[static_cast<size_t>(i)];
        p.sin_family = AF_INET;
        p.sin_port = htons(static_cast<uint16_t>(base + i));
        p.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    std::printf("[switch %6ld ms] %d ports, run %ld ms", 0L, nodes, run_ms);
    if (cut) std::printf(", partition {%s} | {%s} from %ld to %ld ms", group_a.c_str(), group_b.c_str(), cut_from, cut_to);
    std::printf("\n");
    std::fflush(stdout);

    std::signal(SIGTERM, on_term);
    const auto t0 = std::chrono::steady_clock::now();
    auto ms = [&] {
        return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count());
    };
    std::array<std::array<long, 9>, 9> fwd{}, drop{};
    std::array<long, 9> hb{};
    std::map<std::string, std::pair<long, long>> last_line;   // key -> (time printed, repeats since)
    bool was_cut = false;
    std::vector<pollfd> pfd;
    for (int i = 1; i <= nodes; ++i) pfd.push_back(pollfd{fds[static_cast<size_t>(i)], POLLIN, 0});
    std::array<uint8_t, 2048> buf{};

    while (ms() < run_ms && stop_requested == 0) {
        const long t = ms();
        const bool cut_now = cut && t >= cut_from && t < cut_to;
        if (cut_now != was_cut) {
            std::printf("[switch %6ld ms] partition %s\n", t, cut_now ? "STARTS" : "HEALED");
            if (cut_now) std::printf("[switch %6ld ms]   group A {%s}, group B {%s}: no frames cross\n", t, group_a.c_str(), group_b.c_str());
            std::fflush(stdout);
            was_cut = cut_now;
        }
        if (poll(pfd.data(), pfd.size(), 20) <= 0) continue;
        for (int i = 1; i <= nodes; ++i) {
            if ((pfd[static_cast<size_t>(i - 1)].revents & POLLIN) == 0) continue;
            ssize_t n = recv(fds[static_cast<size_t>(i)], buf.data(), buf.size(), 0);
            if (n < 14) continue;
            bool ours = n >= 26 && buf[12] == 0x88 && buf[13] == 0xB5 && buf[14] == 'D';
            int type = ours ? buf[18] : 0, to = ours ? buf[17] : 0;
            uint32_t term = 0;
            if (ours) std::memcpy(&term, &buf[22], 4);
            if (type == 1) ++hb[static_cast<size_t>(i)];
            std::string dropped;
            for (int j = 1; j <= nodes; ++j) {
                if (j == i) continue;
                bool blocked = cut_now && group_of(i, group_a) != group_of(j, group_a);
                if (blocked) {
                    ++drop[static_cast<size_t>(i)][static_cast<size_t>(j)];
                    if (to == 0 || to == j) dropped += " n" + std::to_string(j);
                } else {
                    ++fwd[static_cast<size_t>(i)][static_cast<size_t>(j)];
                    sendto(fds[static_cast<size_t>(j)], buf.data(), static_cast<size_t>(n), 0,
                           reinterpret_cast<sockaddr*>(&peer[static_cast<size_t>(j)]), sizeof(sockaddr_in));
                }
            }
            // Log every cluster message except the periodic ones: heartbeats, append replies,
            // and Raft appends that carry no entries (byte 38 = entry count; naive mode uses term 0).
            bool quiet = type == 1 || type == 23 || (type == 22 && term != 0 && n > 38 && buf[38] == 0);
            // The same message kind on the same path is printed at most once per 2 s.
            std::string key = std::to_string(i) + ">" + std::to_string(to) + ":" + std::to_string(type) + ":" +
                              std::to_string(term) + ":" + dropped;
            auto& [printed, repeats] = last_line[key];
            if (ours && !quiet && printed != 0 && t - printed < 2000) {
                ++repeats;
            } else if (ours && !quiet) {
                std::printf("[switch %6ld ms] n%d -> %s%s %-10s term %u%s%s", t, i, to == 0 ? "all" : "n",
                            to == 0 ? "" : std::to_string(to).c_str(), type_name(type), term,
                            dropped.empty() ? "" : "  DROPPED for", dropped.c_str());
                if (repeats > 0) std::printf("  (+%ld similar since the last printed line)", repeats);
                std::printf("\n");
                std::fflush(stdout);
                printed = t == 0 ? 1 : t;
                repeats = 0;
            }
        }
    }
    std::printf("[switch %6ld ms] stop. heartbeats received:", ms());
    for (int i = 1; i <= nodes; ++i) std::printf(" n%d=%ld", i, hb[static_cast<size_t>(i)]);
    std::printf("\nframes forwarded / dropped (from -> to):\n");
    for (int i = 1; i <= nodes; ++i)
        for (int j = 1; j <= nodes; ++j)
            if (i != j) std::printf("  n%d -> n%d  %ld / %ld\n", i, j, fwd[static_cast<size_t>(i)][static_cast<size_t>(j)], drop[static_cast<size_t>(i)][static_cast<size_t>(j)]);
    for (int i = 1; i <= nodes; ++i) close(fds[static_cast<size_t>(i)]);
    return 0;
}
