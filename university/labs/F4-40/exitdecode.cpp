// exitdecode.cpp - decode VM-exit codes of both x86 virtualization extensions with the
// names Linux's UAPI headers give them (<asm/svm.h>, <asm/vmx.h>, linux-libc-dev 6.8.0).
// Input lines: "svm <exit code>", "vmx <exit reason>" or "ioio <EXITINFO1>" (hex).
#include <asm/kvm.h>   // defines DE_VECTOR ... used inside SVM_EXIT_REASONS
#include <asm/svm.h>
#include <asm/vmx.h>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
struct Name { long long code; const char* name; };
const Name kSvm[] = { SVM_EXIT_REASONS };
const Name kVmx[] = { VMX_EXIT_REASONS };

const char* lookup(const Name* table, std::size_t n, long long code)
{
    for (std::size_t i = 0; i < n; ++i) {
        if (table[i].code == code) {
            return table[i].name;
        }
    }
    return "(not in the header)";
}
}

int main()
{
    std::string kind;
    std::string hex;
    while (std::cin >> kind >> hex) {
        const std::uint64_t v = std::stoull(hex, nullptr, 16);
        std::cout << kind << ' ' << hex << ": ";
        if (kind == "svm") {
            // The exit code is a 64-bit field; -1 means VMRUN failed its checks.
            long long code = static_cast<long long>(v);
            if (v == 0xffffffffULL) {
                std::cout << "(only the low 32 bits are set) ";
                code = -1;
            }
            std::cout << lookup(kSvm, std::size(kSvm), code) << '\n';
        } else if (kind == "vmx") {
            // Bits 15:0 are the basic exit reason; bit 31 says the VM entry itself failed.
            const bool failed = v & VMX_EXIT_REASONS_FAILED_VMENTRY;
            std::cout << "basic reason " << (v & 0xffff) << " = "
                      << lookup(kVmx, std::size(kVmx), static_cast<long long>(v & 0xffff))
                      << (failed ? ", VM-entry failure (bit 31 set)" : "") << '\n';
        } else if (kind == "ioio") {
            // EXITINFO1 of an IOIO exit (layout from the AMD APM, not opened in this build):
            // bit 0 = IN, bit 2 = string, bits 4-6 = operand size, bits 31:16 = port.
            std::cout << ((v & 1) ? "IN" : "OUT") << ((v & 4) ? " (string)" : "")
                      << ", size " << ((v & 0x10) ? 1 : (v & 0x20) ? 2 : 4) << " byte(s)"
                      << ", port 0x" << std::hex << ((v >> 16) & 0xffff) << std::dec << '\n';
        } else {
            std::cout << "unknown kind\n";
        }
    }
    return 0;
}
