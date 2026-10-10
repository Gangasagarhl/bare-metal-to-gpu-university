// acpi_ids.cpp - find the hardware IDs (_HID) and compatible IDs (_CID) in a DSDT without an
// AML interpreter: look for the AML encoding of Name(_HID, ...) and Name(_CID, ...), decode the
// value (a compressed EISA ID or a string), and compare with what Linux reports, if asked.
//   acpi_ids                      reads this machine's DSDT and compares with /sys/bus/acpi
//   acpi_ids <file>               reads a DSDT saved in a file (for example from QEMU)
// The AML encodings assumed here (NameOp 0x08, DWordPrefix 0x0C, StringPrefix 0x0D) and the EISA
// ID compression must be checked against the ACPI Specification (title only in this build); the
// comparison with Linux's own list below is how this program checks itself.
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

static std::string eisa_decode(const unsigned char* b)
{
    // Three letters of 5 bits each (bit 15 is zero), then four hexadecimal digits.
    std::string s;
    s += static_cast<char>('@' + ((b[0] >> 2) & 0x1F));
    s += static_cast<char>('@' + (((b[0] & 0x3) << 3) | (b[1] >> 5)));
    s += static_cast<char>('@' + (b[1] & 0x1F));
    const char* hx = "0123456789ABCDEF";
    s += hx[b[2] >> 4];
    s += hx[b[2] & 0xF];
    s += hx[b[3] >> 4];
    s += hx[b[3] & 0xF];
    return s;
}

int main(int argc, char** argv)
{
    const std::string path = argc > 1 ? argv[1] : "/sys/firmware/acpi/tables/DSDT";
    std::ifstream f(path, std::ios::binary);
    const std::vector<unsigned char> aml((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (aml.size() < 36) {
        std::cout << "cannot read a DSDT from " << path << '\n';
        return 1;
    }
    unsigned sum = 0;
    for (unsigned char c : aml) sum += c;
    const uint32_t len = aml[4] | aml[5] << 8 | aml[6] << 16 | static_cast<uint32_t>(aml[7]) << 24;
    std::cout << "table " << std::string(aml.begin(), aml.begin() + 4) << ", length field " << len
              << ", bytes read " << aml.size() << ", checksum " << ((sum & 0xFF) == 0 ? "ok" : "BAD")
              << ", OEM ID \"" << std::string(aml.begin() + 10, aml.begin() + 16) << "\"\n";

    std::map<std::string, int> found;        // "kind id" -> how many times
    for (std::size_t i = 36; i + 10 < aml.size(); ++i) {
        if (aml[i] != 0x08 || aml[i + 1] != '_' || aml[i + 3] != 'I' || aml[i + 4] != 'D') continue;
        const char which = static_cast<char>(aml[i + 2]);
        if (which != 'H' && which != 'C') continue;
        const std::string kind = which == 'H' ? "_HID" : "_CID";
        const std::size_t v = i + 5;
        if (aml[v] == 0x0C) {
            ++found[kind + " " + eisa_decode(&aml[v + 1])];
        } else if (aml[v] == 0x0D) {
            std::string s;
            for (std::size_t j = v + 1; j < aml.size() && aml[j] != 0 && s.size() < 16; ++j)
                s += static_cast<char>(aml[j]);
            ++found[kind + " " + s];
        } else {
            ++found[kind + " (value is not a constant: a package or a method)"];
        }
    }
    std::cout << "IDs found in the AML (count = how many devices declare it):\n";
    for (const auto& [id, n] : found) std::cout << "  " << id << "  x" << n << '\n';

    if (argc > 1) return 0;
    std::set<std::string> linux_hids;
    for (const auto& e : std::filesystem::directory_iterator("/sys/bus/acpi/devices")) {
        std::ifstream h(e.path() / "hid");
        std::string s;
        if (std::getline(h, s) && !s.empty()) linux_hids.insert(s);
    }
    std::cout << "compared with the hardware IDs Linux reports in /sys/bus/acpi/devices:\n";
    for (const auto& id : linux_hids) {
        const bool in_aml = found.count("_HID " + id) > 0;
        std::cout << "  " << id << (in_aml ? "  found in the DSDT" : "  not in the DSDT (Linux-made or from another table)")
                  << '\n';
    }
    return 0;
}
