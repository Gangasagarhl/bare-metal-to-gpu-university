// show_addrs.cpp - list this machine's IPv4 interface addresses and their prefix lengths.
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main()
{
    ifaddrs* list = nullptr;
    if (getifaddrs(&list) != 0) {
        std::printf("getifaddrs failed: %s\n", std::strerror(errno));
        return 1;
    }
    for (const ifaddrs* i = list; i != nullptr; i = i->ifa_next) {
        if (i->ifa_addr == nullptr || i->ifa_addr->sa_family != AF_INET) {
            continue;                                   // only IPv4 entries
        }
        const auto* a = reinterpret_cast<const sockaddr_in*>(i->ifa_addr);
        const auto* m = reinterpret_cast<const sockaddr_in*>(i->ifa_netmask);
        char addr[INET_ADDRSTRLEN];
        char mask[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &a->sin_addr, addr, sizeof addr);
        inet_ntop(AF_INET, &m->sin_addr, mask, sizeof mask);
        const int prefix = std::popcount(ntohl(m->sin_addr.s_addr));   // count the one bits
        std::printf("%-6s %s/%d (mask %s)\n", i->ifa_name, addr, prefix, mask);
    }
    freeifaddrs(list);
    return 0;
}
