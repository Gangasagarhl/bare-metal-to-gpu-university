// msicheck.cpp - DR302 F4-08: compares the kernel's MSI/MSI-X constants (msi.h) with the
// values in Linux's <linux/pci_regs.h> installed in the build container (a tier-4 source:
// what that implementation defines). A mismatch makes the program fail.
#include <cstdio>
#include <linux/pci_regs.h>
#include "msi.h"

namespace
{
int g_bad = 0;

void check(const char* name, unsigned ours, unsigned linux_value)
{
    const bool ok = ours == linux_value;
    std::printf("%-22s ours 0x%04x  linux 0x%04x  %s\n", name, ours, linux_value, ok ? "ok" : "MISMATCH");
    if (!ok) {
        ++g_bad;
    }
}
}  // namespace

int main()
{
    using namespace msireg;
    check("cap id MSI", pcireg::CAP_MSI, PCI_CAP_ID_MSI);
    check("cap id MSI-X", pcireg::CAP_MSIX, PCI_CAP_ID_MSIX);
    check("MSI flags", MSI_FLAGS, PCI_MSI_FLAGS);
    check("MSI enable", MSI_ENABLE, PCI_MSI_FLAGS_ENABLE);
    check("MSI multiple capable", MSI_QMASK, PCI_MSI_FLAGS_QMASK);
    check("MSI multiple enable", MSI_QSIZE, PCI_MSI_FLAGS_QSIZE);
    check("MSI 64-bit", MSI_64BIT, PCI_MSI_FLAGS_64BIT);
    check("MSI per-vector mask", MSI_MASKBIT, PCI_MSI_FLAGS_MASKBIT);
    check("MSI address low", MSI_ADDR_LO, PCI_MSI_ADDRESS_LO);
    check("MSI address high", MSI_ADDR_HI, PCI_MSI_ADDRESS_HI);
    check("MSI data (32-bit)", MSI_DATA_32, PCI_MSI_DATA_32);
    check("MSI data (64-bit)", MSI_DATA_64, PCI_MSI_DATA_64);
    check("MSI mask (64-bit)", MSI_MASK_64, PCI_MSI_MASK_64);
    check("MSI-X flags", MSIX_FLAGS, PCI_MSIX_FLAGS);
    check("MSI-X table size", MSIX_QSIZE, PCI_MSIX_FLAGS_QSIZE);
    check("MSI-X function mask", MSIX_MASKALL, PCI_MSIX_FLAGS_MASKALL);
    check("MSI-X enable", MSIX_ENABLE, PCI_MSIX_FLAGS_ENABLE);
    check("MSI-X table offset", MSIX_TABLE, PCI_MSIX_TABLE);
    check("MSI-X PBA offset", MSIX_PBA, PCI_MSIX_PBA);
    check("MSI-X BIR mask", MSIX_BIR, PCI_MSIX_TABLE_BIR);
    check("MSI-X entry size", ENTRY_SIZE, PCI_MSIX_ENTRY_SIZE);
    check("entry data", ENTRY_DATA, PCI_MSIX_ENTRY_DATA);
    check("entry vector control", ENTRY_CTRL, PCI_MSIX_ENTRY_VECTOR_CTRL);
    check("entry mask bit", ENTRY_MASKED, PCI_MSIX_ENTRY_CTRL_MASKBIT);
    std::printf("%s: %d mismatches\n", g_bad == 0 ? "PASS" : "FAIL", g_bad);
    return g_bad == 0 ? 0 : 1;
}
