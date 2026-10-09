// hvdetect.cpp - am I running in a virtual machine? Ask the processor (CPUID), the way the
// curriculum's milestone V1 says a guest kernel should: the "hypervisor present" bit of
// leaf 1, then the vendor signature in the hypervisor leaf range starting at 0x40000000.
// x86-64 only. Leaf and bit numbers: Intel SDM Vol. 2 (CPUID) and the KVM documentation
// "KVM CPUID bits" (title only, pending verification); the run shows what THIS machine says.
#include <cpuid.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

int main()
{
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    __cpuid(1, eax, ebx, ecx, edx);
    const bool hypervisor = ((ecx >> 31) & 1u) != 0;
    std::cout << "CPUID leaf 1, ECX bit 31 (hypervisor present): " << hypervisor << '\n';
    if (!hypervisor) {
        std::cout << "no hypervisor reported: bare metal, or a hypervisor that hides itself\n";
        return 0;
    }
    __cpuid(0x40000000u, eax, ebx, ecx, edx);
    char sig[13] = {};
    std::memcpy(sig + 0, &ebx, 4);
    std::memcpy(sig + 4, &ecx, 4);
    std::memcpy(sig + 8, &edx, 4);
    std::cout << "CPUID leaf 0x40000000: highest hypervisor leaf 0x" << std::hex << eax << std::dec
              << ", vendor signature \"" << sig << "\"\n";
    const std::string s(sig);
    const char* name = s == "KVMKVMKVM" ? "KVM"
                     : s == "TCGTCGTCGTCG" ? "QEMU without acceleration (TCG)"
                     : s == "Microsoft Hv" ? "Microsoft Hyper-V"
                     : s == "XenVMMXenVMM" ? "Xen"
                     : s == "VMwareVMware" ? "VMware"
                     : "unknown to this program";
    std::cout << "hypervisor: " << name << '\n';
    return 0;
}
