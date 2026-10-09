// cpuid_id.cpp - identify the CPU from the CPUID instruction (stage (a) for a processor) and
// compare with what the Linux kernel reports in /proc/cpuinfo. x86 only.
// Field layout (leaf 0: vendor in EBX, EDX, ECX; leaf 1 EAX: stepping 3:0, model 7:4, family 11:8,
// extended model 19:16, extended family 27:20; leaf 1 ECX bit 31: running under a hypervisor;
// leaf 0x40000000: the hypervisor's vendor signature) is as this chapter states it; check it
// against the Intel SDM, Volume 2, CPUID instruction (title only in this build).
#include <cpuid.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

static std::string regs_to_string(unsigned a, unsigned b, unsigned c)
{
    char s[13];
    std::memcpy(s, &a, 4);
    std::memcpy(s + 4, &b, 4);
    std::memcpy(s + 8, &c, 4);
    s[12] = 0;
    return s;
}

int main()
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    __cpuid(0, a, b, c, d);
    const unsigned max_leaf = a;
    const std::string vendor = regs_to_string(b, d, c);
    __cpuid(1, a, b, c, d);
    unsigned family = (a >> 8) & 0xF, model = (a >> 4) & 0xF;
    const unsigned stepping = a & 0xF;
    if (family == 0xF) family += (a >> 20) & 0xFF;
    if (family == 0x6 || family >= 0xF) model += ((a >> 16) & 0xF) << 4;
    const bool under_hypervisor = (c >> 31) & 1;
    std::cout << "CPUID leaf 0: vendor \"" << vendor << "\", highest standard leaf " << max_leaf << '\n'
              << "CPUID leaf 1: family " << family << ", model " << model << ", stepping " << stepping
              << ", hypervisor bit " << under_hypervisor << '\n';
    if (under_hypervisor) {
        __cpuid(0x40000000, a, b, c, d);
        std::cout << "CPUID leaf 0x40000000: hypervisor signature \"" << regs_to_string(b, c, d) << "\"\n";
    }
    __cpuid(0x80000000, a, b, c, d);
    if (a >= 0x80000004) {
        std::string brand;
        for (unsigned leaf = 0x80000002; leaf <= 0x80000004; ++leaf) {
            __cpuid(leaf, a, b, c, d);
            brand += regs_to_string(a, b, c) + regs_to_string(d, 0, 0).substr(0, 4);
        }
        std::cout << "CPUID leaves 0x80000002-4: brand string \"" << brand.c_str() << "\"\n";
    }

    std::ifstream f("/proc/cpuinfo");
    std::string line;
    std::cout << "/proc/cpuinfo (first CPU) for comparison:\n";
    int shown = 0;
    while (std::getline(f, line) && shown < 5) {
        for (const char* key : {"vendor_id", "cpu family", "model\t", "model name", "stepping"}) {
            if (line.rfind(key, 0) == 0) {
                std::cout << "  " << line << '\n';
                ++shown;
            }
        }
    }
    return 0;
}
