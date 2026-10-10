// hostguest.cpp - is the machine running this program a guest, and does its OS behave
// like a good one? Run inside the build container (a Linux user program): CPUID works
// in user mode, and Linux publishes its clock-source choice and its virtio devices in
// sysfs. Names come from Linux's UAPI headers (linux-libc-dev 6.8.0) in this build.
#include <asm/kvm_para.h>       // KVM_CPUID_SIGNATURE, KVM_FEATURE_*
#include <cpuid.h>              // GCC's __cpuid / __get_cpuid_max
#include <linux/virtio_ids.h>   // VIRTIO_ID_*
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace {
// VIRTIO_RING_F_EVENT_IDX is 29 in <linux/virtio_ring.h> (read in this build). That header
// does not compile as C++ (it converts void* implicitly), so the value is copied here.
constexpr std::size_t kEventIdxBit = 29;

std::string read_line(const std::filesystem::path& p)
{
    std::ifstream f(p);
    std::string s;
    std::getline(f, s);
    return s;
}

const char* virtio_name(unsigned id)
{
    switch (id) {
    case VIRTIO_ID_NET: return "net";
    case VIRTIO_ID_BLOCK: return "block";
    case VIRTIO_ID_CONSOLE: return "console";
    case VIRTIO_ID_RNG: return "rng (entropy)";
    case VIRTIO_ID_BALLOON: return "balloon";
    case VIRTIO_ID_VSOCK: return "vsock";
    case VIRTIO_ID_MEM: return "mem";
    default: return "other";
    }
}
}

int main()
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    __cpuid(1, a, b, c, d);
    const bool hv = (c >> 31) & 1;
    std::cout << "hypervisor-present bit: " << hv << '\n';
    // Can this machine itself run a hardware-assisted hypervisor? VMX is CPUID.1:ECX bit 5,
    // SVM is CPUID 0x80000001:ECX bit 2 (bit positions from memory of the vendor manuals;
    // Linux's /proc/cpuinfo calls them "vmx" and "svm").
    const bool vmx = (c >> 5) & 1;
    __cpuid(0x80000001, a, b, c, d);
    const bool svm = (c >> 2) & 1;
    std::cout << "virtualization extensions offered to this machine: VMX " << (vmx ? "yes" : "no")
              << ", SVM " << (svm ? "yes" : "no") << '\n';
    if (hv) {
        __cpuid(KVM_CPUID_SIGNATURE, a, b, c, d);
        char sig[13] = {};
        std::memcpy(sig, &b, 4);
        std::memcpy(sig + 4, &c, 4);
        std::memcpy(sig + 8, &d, 4);
        std::cout << "CPUID 0x40000000: max leaf 0x" << std::hex << a << std::dec
                  << ", signature \"" << sig << "\"\n";
        if (std::memcmp(sig, KVM_SIGNATURE, 12) == 0 && a >= KVM_CPUID_FEATURES) {
            __cpuid(KVM_CPUID_FEATURES, a, b, c, d);
            std::cout << "KVM features 0x" << std::hex << a << std::dec << ":";
            const struct { int bit; const char* name; } f[] = {
                {KVM_FEATURE_CLOCKSOURCE, "CLOCKSOURCE"}, {KVM_FEATURE_CLOCKSOURCE2, "CLOCKSOURCE2"},
                {KVM_FEATURE_STEAL_TIME, "STEAL_TIME"}, {KVM_FEATURE_PV_EOI, "PV_EOI"},
                {KVM_FEATURE_PV_UNHALT, "PV_UNHALT"}, {KVM_FEATURE_PV_TLB_FLUSH, "PV_TLB_FLUSH"},
                {KVM_FEATURE_PV_SEND_IPI, "PV_SEND_IPI"}, {KVM_FEATURE_POLL_CONTROL, "POLL_CONTROL"},
                {KVM_FEATURE_PV_SCHED_YIELD, "PV_SCHED_YIELD"},
                {KVM_FEATURE_CLOCKSOURCE_STABLE_BIT, "CLOCKSOURCE_STABLE_BIT"}};
            for (const auto& x : f) {
                if ((a >> x.bit) & 1) {
                    std::cout << ' ' << x.name;
                }
            }
            std::cout << '\n';
        }
    }
    const std::filesystem::path cs = "/sys/devices/system/clocksource/clocksource0";
    std::cout << "clock sources offered: " << read_line(cs / "available_clocksource") << '\n';
    std::cout << "clock source in use:   " << read_line(cs / "current_clocksource") << '\n';
    const std::filesystem::path bus = "/sys/bus/virtio/devices";
    std::error_code ec;
    std::vector<std::filesystem::path> devs;
    for (const auto& dev : std::filesystem::directory_iterator(bus, ec)) {
        devs.push_back(dev.path());
    }
    std::sort(devs.begin(), devs.end());
    for (const auto& path : devs) {
        const std::filesystem::directory_entry dev(path);
        unsigned id = std::stoul(read_line(dev.path() / "device"), nullptr, 16);
        std::string features = read_line(dev.path() / "features");   // one '0'/'1' per bit
        bool event_idx = features.size() > kEventIdxBit && features[kEventIdxBit] == '1';
        std::string driver = std::filesystem::read_symlink(dev.path() / "driver", ec).filename();
        std::cout << dev.path().filename().string() << ": id " << id << " = " << virtio_name(id)
                  << ", driver " << driver << ", EVENT_IDX " << (event_idx ? "yes" : "no") << '\n';
    }
    if (ec) {
        std::cout << "no virtio devices visible (" << ec.message() << ")\n";
    }
    return 0;
}
