// F9-14 Listing 2: try to open a real SocketCAN socket, as a robot computer with a CAN
// interface would. In the build container there is no CAN hardware or CAN support, so
// this run shows the operating system's real answer. Untested on hardware.
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

int main()
{
    const int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        std::printf("socket(PF_CAN, SOCK_RAW, CAN_RAW) failed: %s\n", std::strerror(errno));
        std::printf("no CAN support here: this program needs a machine with a CAN interface\n");
        return 0;
    }
    const unsigned idx = if_nametoindex("can0");
    if (idx == 0) {
        std::printf("socket opened, but no interface named can0: %s\n", std::strerror(errno));
    } else {
        std::printf("can0 has interface index %u\n", idx);
    }
    close(s);
    return 0;
}
