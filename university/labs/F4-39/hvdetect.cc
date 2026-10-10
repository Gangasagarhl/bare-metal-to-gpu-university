// hvdetect.cc - hypervisor detection through the CPUID hypervisor range.
// KVM's leaf numbers and signature come from <asm/kvm_para.h> (linux-libc-dev 6.8.0,
// opened in this build); "TCGTCGTCGTCG" was printed by QEMU 8.2.2 in this build's run.
// The other signatures are from the curriculum's table 12.3 and are unverified here.
#include "hvdetect.h"
#include "kio.h"

namespace {
struct Known { const char* sig; const char* name; };
constexpr Known kKnown[] = {
    {"KVMKVMKVM\0\0\0", "KVM"},
    {"TCGTCGTCGTCG", "QEMU TCG (no accelerator)"},
    {"Microsoft Hv", "Microsoft Hyper-V (unverified signature)"},
    {"XenVMMXenVMM", "Xen (unverified signature)"},
    {"VMwareVMware", "VMware (unverified signature)"},
    {"DR404-tinyHV", "the DR404 tiny hypervisor (F4-41)"},
};
}

HvInfo detect_hypervisor()
{
    HvInfo info{};
    info.name = "none";
    info.present = (cpuid(1).ecx >> 31) & 1;
    if (!info.present) {
        return info;              // bare metal, or a hypervisor that hides itself
    }
    CpuidRegs r = cpuid(0x40000000);
    info.max_leaf = r.eax;
    memcpy(info.signature + 0, &r.ebx, 4);
    memcpy(info.signature + 4, &r.ecx, 4);
    memcpy(info.signature + 8, &r.edx, 4);
    info.signature[12] = 0;
    info.name = "unknown";
    for (const Known& k : kKnown) {
        if (memcmp(info.signature, k.sig, 12) == 0) {
            info.name = k.name;
        }
    }
    if (memcmp(info.signature, "KVMKVMKVM\0\0\0", 12) == 0 && info.max_leaf >= 0x40000001) {
        info.kvm_features = cpuid(0x40000001).eax;
    }
    return info;
}
