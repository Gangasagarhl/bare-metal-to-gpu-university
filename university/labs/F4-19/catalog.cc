// catalog.cc - stage (b)'s entry point: put every device of a machine into a row of the
// hardware catalog (curriculum section 6.1), which names the standard to read and the open
// drivers to compare with. Input on standard input: records from mk_machines.py
//   machine|kind|id|class or compatible strings|where
// The rows and the matching keys below are taken from the catalog's own text (a map, not a
// source); a device that matches no key is reported as "unclassified" rather than guessed.
// Milestone column: the milestone the catalog row names; where the row names none, the Track B,
// C or D milestone whose title names the device (C1 PCI, C2 UART and PS/2, C3 RTC, C4 virtio,
// C6 NVMe, B6 serial console); "-" where neither applies.
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Row { const char* name; const char* standard; const char* milestone; };

static const Row kRows[] = {
    {"CPUs and SMP", "architecture manuals (Intel SDM, AMD64 APM, Arm ARM, RISC-V)", "B11, D3, D6"},
    {"Interrupt controllers: Arm", "Arm Generic Interrupt Controller Architecture Specification", "D2"},
    {"Timers and clock sources", "HPET Specification; Arm Generic Timer; ...", "B8"},
    {"Platform power on Arm: PSCI and SCMI", "Arm PSCI", "-"},
    {"Clocks, resets and power domains (SoCs)", "SoC TRMs; devicetree clock bindings", "-"},
    {"PCI and PCI Express", "PCI Local Bus / PCI Express Base Specifications", "C1, C7"},
    {"USB host controllers", "xHCI Specification; USB 2.0 / 3.2", "C8"},
    {"I2C, SMBus, I3C and sensors", "NXP UM10204; SMBus Specification", "H2, C14"},
    {"GPIO and pin control", "SoC TRM GPIO chapters; ACPI GpioIo/GpioInt", "-"},
    {"Serial UARTs and early consoles", "PC16550D datasheet; Arm PL011 TRM", "B6, C2"},
    {"AHCI (SATA)", "Serial ATA AHCI Specification", "C5"},
    {"NVMe", "NVM Express specifications", "C6"},
    {"virtio storage", "OASIS VIRTIO Specification", "C4"},
    {"virtio 1.x, the full family", "OASIS VIRTIO Specification", "C4"},
    {"Ethernet: Intel controllers", "Intel 8254x SDM and datasheets", "C9, M2"},
    {"Input: PS/2 and USB HID", "i8042 conventions; HID class definition", "C2, C8"},
    {"Audio", "High Definition Audio Specification", "C11"},
    {"RTC", "MC146818; Arm PL031 TRM; ACPI Time and Alarm", "C3"},
    {"(chipset bridges, inventory only)", "chipset datasheet", "-"},
};

static const Row* row(const char* name)
{
    for (const Row& r : kRows)
        if (std::string(r.name) == name) return &r;
    return nullptr;
}

static const Row* classify(const std::string& kind, const std::string& id, const std::string& klass)
{
    if (kind == "pci") {
#ifdef F419_BASE_ONLY
        const std::string key = klass.substr(0, 2);         // the forensic build: base class only
#else
        const std::string key = klass;
#endif
        if (id.rfind("1af4:", 0) == 0) return klass.rfind("01", 0) == 0 ? row("virtio storage") : row("virtio 1.x, the full family");
        static const std::map<std::string, const char*> by_class = {
            {"01", "AHCI (SATA)"},                     // reached only by the forensic build
            {"010601", "AHCI (SATA)"},
            {"010802", "NVMe"},
            {"0c0330", "USB host controllers"},
            {"0c0500", "I2C, SMBus, I3C and sensors"},
            {"020000", "Ethernet: Intel controllers"},
            {"040300", "Audio"},
            {"060000", "(chipset bridges, inventory only)"},
            {"060100", "(chipset bridges, inventory only)"},
        };
        const auto it = by_class.find(key);
        if (it != by_class.end()) {
            if (std::string(it->second) == "Ethernet: Intel controllers" && id.rfind("8086:", 0) != 0) return nullptr;
            return row(it->second);
        }
        return nullptr;
    }
    if (kind == "acpi") {
        static const std::map<std::string, const char*> by_hid = {
            {"PNP0501", "Serial UARTs and early consoles"}, {"PNP0303", "Input: PS/2 and USB HID"},
            {"PNP0A08", "PCI and PCI Express"}, {"PNP0B00", "RTC"}, {"PNP0103", "Timers and clock sources"},
        };
        const auto it = by_hid.find(id);
        return it == by_hid.end() ? nullptr : row(it->second);
    }
    // devicetree: the first (most specific) compatible string decides
    static const std::map<std::string, const char*> by_compat = {
        {"arm,pl011", "Serial UARTs and early consoles"}, {"arm,pl031", "RTC"},
        {"arm,pl061", "GPIO and pin control"}, {"arm,cortex-a15-gic", "Interrupt controllers: Arm"},
        {"arm,gic-v2m-frame", "Interrupt controllers: Arm"}, {"arm,armv8-timer", "Timers and clock sources"},
        {"arm,psci-1.0", "Platform power on Arm: PSCI and SCMI"}, {"virtio,mmio", "virtio 1.x, the full family"},
        {"pci-host-ecam-generic", "PCI and PCI Express"}, {"fixed-clock", "Clocks, resets and power domains (SoCs)"},
        {"arm,cortex-a57", "CPUs and SMP"}, {"arm,armv8-pmuv3", "CPUs and SMP"},
    };
    const auto it = by_compat.find(id);
    return it == by_compat.end() ? nullptr : row(it->second);
}

int main()
{
    std::string line, current;
    std::map<std::string, std::map<std::string, int>> rows_per_machine;
    int unclassified = 0, total = 0;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '|')) f.push_back(x);
        if (f.size() != 5) continue;
        if (f[0] != current) {
            current = f[0];
            std::cout << "== machine " << current << " ==\n";
        }
        const Row* r = classify(f[1], f[2], f[3]);
        ++total;
        std::cout << "  " << f[1] << " " << f[2] << " (" << f[3] << ") at " << f[4] << "\n      -> ";
        if (r) {
            std::cout << r->name << "  [read: " << r->standard << "; milestone " << r->milestone << "]\n";
            ++rows_per_machine[current][r->name];
        } else {
            std::cout << "UNCLASSIFIED: no catalog key matches; look it up by hand\n";
            ++unclassified;
        }
    }
    std::cout << "== catalog rows touched per machine ==\n";
    for (const auto& [m, rows] : rows_per_machine) {
        std::cout << m << ": " << rows.size() << " rows:";
        for (const auto& [name, n] : rows) std::cout << " [" << name << " x" << n << "]";
        std::cout << '\n';
    }
    std::cout << total << " devices, " << unclassified << " unclassified\n";
    return 0;
}
