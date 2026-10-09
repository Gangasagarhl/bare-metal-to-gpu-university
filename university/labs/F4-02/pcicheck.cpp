// pcicheck.cpp - DR301 F4-02: the configuration-space constants of pci.h (written from the
// author's memory of the PCI Local Bus Specification) against an independent source,
// Linux's <linux/pci_regs.h> as installed in the build container.
#include <cstdio>
#include <linux/pci_regs.h>
#include "pci.h"

static int failures = 0;

static void check(const char* name, unsigned ours, unsigned linux_value)
{
    const bool same = ours == linux_value;
    if (!same) ++failures;
    std::printf("%-18s ours 0x%03x  linux 0x%03x  %s\n", name, ours, linux_value, same ? "same" : "DIFFERENT");
}

int main()
{
    check("VENDOR_ID", pcireg::VENDOR_ID, PCI_VENDOR_ID);
    check("DEVICE_ID", pcireg::DEVICE_ID, PCI_DEVICE_ID);
    check("COMMAND", pcireg::COMMAND, PCI_COMMAND);
    check("STATUS", pcireg::STATUS, PCI_STATUS);
    check("CLASS_REVISION", pcireg::CLASS_REVISION, PCI_CLASS_REVISION);
    check("HEADER_TYPE", pcireg::HEADER_TYPE, PCI_HEADER_TYPE);
    check("BAR0", pcireg::BAR0, PCI_BASE_ADDRESS_0);
    check("PRIMARY_BUS", pcireg::PRIMARY_BUS, PCI_PRIMARY_BUS);
    check("SECONDARY_BUS", pcireg::SECONDARY_BUS, PCI_SECONDARY_BUS);
    check("SUBORDINATE_BUS", pcireg::SUBORDINATE_BUS, PCI_SUBORDINATE_BUS);
    check("CAP_PTR", pcireg::CAP_PTR, PCI_CAPABILITY_LIST);
    check("INTERRUPT_LINE", pcireg::INTERRUPT_LINE, PCI_INTERRUPT_LINE);
    check("INTERRUPT_PIN", pcireg::INTERRUPT_PIN, PCI_INTERRUPT_PIN);
    check("CMD_IO", pcireg::CMD_IO, PCI_COMMAND_IO);
    check("CMD_MEMORY", pcireg::CMD_MEMORY, PCI_COMMAND_MEMORY);
    check("CMD_MASTER", pcireg::CMD_MASTER, PCI_COMMAND_MASTER);
    check("CMD_INTX_DISABLE", pcireg::CMD_INTX_DISABLE, PCI_COMMAND_INTX_DISABLE);
    check("STATUS_CAP_LIST", pcireg::STATUS_CAP_LIST, PCI_STATUS_CAP_LIST);
    check("HDR_BRIDGE", pcireg::HDR_BRIDGE, PCI_HEADER_TYPE_BRIDGE);
    check("BAR_IO", pcireg::BAR_IO, PCI_BASE_ADDRESS_SPACE_IO);
    check("BAR_TYPE_64", pcireg::BAR_TYPE_64, PCI_BASE_ADDRESS_MEM_TYPE_64);
    check("BAR_PREFETCH", pcireg::BAR_PREFETCH, PCI_BASE_ADDRESS_MEM_PREFETCH);
    check("CAP_PM", pcireg::CAP_PM, PCI_CAP_ID_PM);
    check("CAP_MSI", pcireg::CAP_MSI, PCI_CAP_ID_MSI);
    check("CAP_VENDOR", pcireg::CAP_VENDOR, PCI_CAP_ID_VNDR);
    check("CAP_EXP", pcireg::CAP_EXP, PCI_CAP_ID_EXP);
    check("CAP_MSIX", pcireg::CAP_MSIX, PCI_CAP_ID_MSIX);
    check("EXT_CAP_START", pcireg::EXT_CAP_START, PCI_CFG_SPACE_SIZE);
    std::printf("%d difference(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
