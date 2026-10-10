// F8-09 Listing 2: list the RDMA devices that libibverbs can see, with the attribute
// a two-node GPU job cares about first: the link layer of port 1 (InfiniBand or Ethernet,
// that is RoCE). Built by run.sh with -libverbs (the .cc name keeps the generic runner,
// which links no extra libraries, from building it).
#include <infiniband/verbs.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

int main()
{
    int num = 0;
    errno = 0;
    ibv_device** list = ibv_get_device_list(&num);
    if (list == nullptr) {
        std::printf("ibv_get_device_list failed: errno %d (%s)\n", errno, std::strerror(errno));
        return 2;
    }
    std::printf("RDMA devices found: %d (errno after the call: %d, %s)\n", num, errno,
                std::strerror(errno));
    for (int i = 0; i < num; ++i) {
        ibv_context* ctx = ibv_open_device(list[i]);
        if (ctx == nullptr) {
            std::printf("  %s: cannot open\n", ibv_get_device_name(list[i]));
            continue;
        }
        ibv_port_attr port{};
        if (ibv_query_port(ctx, 1, &port) == 0) {
            std::printf("  %s: port 1 state %d, link layer %s\n", ibv_get_device_name(list[i]),
                        static_cast<int>(port.state),
                        port.link_layer == IBV_LINK_LAYER_ETHERNET ? "Ethernet (RoCE)"
                                                                   : "InfiniBand");
        }
        ibv_close_device(ctx);
    }
    ibv_free_device_list(list);
    return num > 0 ? 0 : 2;
}
