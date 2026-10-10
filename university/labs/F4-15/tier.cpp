// tier.cpp - stage (b), "find documentation", as a fixed decision procedure: for each device
// record, check the six documentation tiers in the curriculum's order and stop at the first
// that applies. Reads tier.in (standard input); prints tier, the rule that fired, the first
// document to read and what the tier means for the plan.
// The class-standard table holds the curriculum's own examples (section 5.2); the normative
// lists are the PCI Code and ID Assignment Specification and the USB-IF "Defined Class Codes"
// (title only in this build).
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Record {
    std::string bus, where, ids, klass, vendor_docs, open_driver, closed_driver, locked, note;
};

struct Standard {
    const char* bus;
    const char* code;      // PCI: 6 hex digits ("xx" = any prog-if); USB: 2 hex digits
    const char* document;
};

static const Standard kStandards[] = {
    {"pci", "010802", "NVM Express Base Specification (NVM Express, Inc.)"},
    {"pci", "010601", "Serial ATA AHCI Specification (Intel)"},
    {"pci", "0c0330", "xHCI Specification (Intel)"},
    {"pci", "0c0320", "Enhanced Host Controller Interface Specification (Intel)"},
    {"pci", "0c0310", "OpenHCI specification"},
    {"pci", "0c0300", "UHCI Design Guide (Intel)"},
    {"pci", "0403xx", "High Definition Audio Specification (Intel)"},
    {"usb", "03", "Device Class Definition for HID (USB-IF)"},
    {"usb", "08", "USB Mass Storage Class specifications (USB-IF)"},
    {"usb", "02", "Class Definitions for Communications Devices (USB-IF)"},
    {"usb", "0a", "Class Definitions for Communications Devices (USB-IF)"},
    {"usb", "01", "USB Audio Class (USB-IF)"},
    {"usb", "0e", "USB Video Class (USB-IF)"},
    {"usb", "09", "USB 2.0 Specification, hub chapter (USB-IF)"},
    {"usb", "0b", "Smart Card CCID class (USB-IF)"},
    {"usb", "e0", "Bluetooth Core Specification, HCI (Bluetooth SIG)"},
};

static const char* standard_for(const Record& r)
{
    if (r.bus == "pci" && r.ids.rfind("1af4:", 0) == 0) return "OASIS VIRTIO Specification (vendor 1AF4 marks virtio)";
    for (const Standard& s : kStandards) {
        if (r.bus != s.bus) continue;
        const std::string code = s.code;
        if (code.size() != r.klass.size()) continue;
        bool same = true;
        for (std::size_t i = 0; i < code.size(); ++i)
            if (code[i] != 'x' && code[i] != r.klass[i]) same = false;
        if (same) return s.document;
    }
    return nullptr;
}

int main()
{
    std::string line;
    int counts[7] = {};
    int undetermined = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, '|')) f.push_back(field);
        if (f.size() != 9) {
            std::cout << "skipped (needs 9 fields): " << line << '\n';
            continue;
        }
        const Record r{f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]};
        int tier = 0;
        std::string rule, first, plan;
        if (const char* doc = standard_for(r)) {
            tier = 1; rule = "an open class standard applies"; first = doc;
            plan = "write the class driver once; collect quirks per device";
        } else if (r.vendor_docs == "public") {
            tier = 2; rule = "the vendor publishes documentation"; first = "the vendor's datasheet or PRM, and its errata";
            plan = "follow the datasheet's initialization chapter; keep the errata at hand";
        } else if (r.vendor_docs == "nda") {
            tier = 3; rule = "documentation exists only under NDA"; first = "the open drivers (never the NDA document)";
            plan = "treat it as tier 4: never use leaked or NDA material";
        } else if (r.vendor_docs == "unknown") {
            rule = "UNDETERMINED: vendor documents not searched yet (step 2 comes before steps 3-6)";
            first = "search for the vendor's public documents first";
        } else if (r.locked == "yes") {
            tier = 6; rule = "locked"; first = "none";
            plan = "record \"unsupported, reason\" in the device report and move on";
        } else if (r.open_driver == "yes") {
            tier = 4; rule = "only open-source drivers exist"; first = "the Linux driver, then the BSD drivers";
            plan = "read the drivers, write your own specification, implement from it";
        } else if (r.closed_driver == "yes") {
            tier = 5; rule = "only a closed driver exists"; first = "your own observation of that driver (stage c)";
            plan = "lawful observation; publish your specification, not their code";
        } else {
            rule = "UNDETERMINED: no driver of any kind recorded"; first = "check the record";
        }
        std::cout << r.bus << " " << r.where << "  " << r.ids << "  class " << r.klass << '\n';
        if (tier) {
            std::cout << "    tier " << tier << ": " << rule << '\n'
                      << "    first document: " << first << '\n'
                      << "    plan: " << plan << '\n';
            ++counts[tier];
        } else {
            std::cout << "    " << rule << '\n' << "    next step: " << first << '\n';
            ++undetermined;
        }
    }
    std::cout << "summary:";
    for (int t = 1; t <= 6; ++t) std::cout << " tier " << t << ": " << counts[t] << ';';
    std::cout << " undetermined: " << undetermined << '\n';
    return 0;
}
