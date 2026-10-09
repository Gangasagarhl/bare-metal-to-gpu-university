// hvdetect.h - which hypervisor are we running under? (curriculum milestone V1)
#pragma once
#include <stdint.h>

struct HvInfo {
    bool present;          // CPUID.1:ECX bit 31, the hypervisor-present bit
    uint32_t max_leaf;     // EAX of CPUID 0x40000000: highest hypervisor leaf
    char signature[13];    // EBX, ECX, EDX of CPUID 0x40000000 as 12 characters
    const char* name;      // our name for the signature, or "unknown"
    uint32_t kvm_features; // EAX of CPUID 0x40000001 when the signature is KVM's
};

HvInfo detect_hypervisor();
