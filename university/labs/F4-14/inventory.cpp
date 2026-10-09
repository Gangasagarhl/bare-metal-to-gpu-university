// inventory.cpp - stage (a) on the machine this program runs on (Linux):
// every PCI function and every ACPI device with a hardware ID, named through the local
// ID databases (pci.ids for PCI, pnp.ids for three-letter PNP vendor prefixes), plus the
// driver Linux bound to each one. Read only: it opens files under /sys and /usr/share.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string slurp(const fs::path& p)
{
    std::ifstream f(p);
    std::string s;
    std::getline(f, s);
    return s;
}

static unsigned hexval(const std::string& s)
{
    return static_cast<unsigned>(std::stoul(s, nullptr, 16));
}

struct PciIds {
    std::map<unsigned, std::string> vendor;
    std::map<unsigned, std::string> device;    // key: vendor << 16 | device
    std::map<unsigned, std::string> klass;     // key: base << 16 | sub << 8 | prog-if, 0xFF = any

    explicit PciIds(const std::string& path)
    {
        std::ifstream f(path);
        std::string line;
        unsigned cur_vendor = 0, cur_base = 0, cur_sub = 0;
        bool in_classes = false;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.rfind("C ", 0) == 0) {                       // "C 01  Mass storage controller"
                in_classes = true;
                cur_base = hexval(line.substr(2, 2));
                klass[cur_base << 16 | 0xFFFF] = line.substr(6);
            } else if (in_classes && line.rfind("\t\t", 0) == 0) {  // prog-if
                klass[cur_base << 16 | cur_sub << 8 | hexval(line.substr(2, 2))] = line.substr(6);
            } else if (in_classes && line[0] == '\t') {             // subclass
                cur_sub = hexval(line.substr(1, 2));
                klass[cur_base << 16 | cur_sub << 8 | 0xFF] = line.substr(5);
            } else if (!in_classes && line[0] != '\t' && line.size() > 6) {
                cur_vendor = hexval(line.substr(0, 4));
                vendor[cur_vendor] = line.substr(6);
            } else if (!in_classes && line[0] == '\t' && line[1] != '\t' && line.size() > 7) {
                device[cur_vendor << 16 | hexval(line.substr(1, 4))] = line.substr(7);
            }
        }
    }

    std::string name_of(unsigned v, unsigned d) const
    {
        const auto vi = vendor.find(v);
        const auto di = device.find(v << 16 | d);
        return (vi == vendor.end() ? std::string("(vendor not in pci.ids)") : vi->second) + " / " +
               (di == device.end() ? std::string("(device not in pci.ids)") : di->second);
    }

    // "base / subclass / programming interface", each looked up on its own level.
    std::string class_of(unsigned c) const
    {
        std::string out;
        for (unsigned key : {(c & 0xFF0000) | 0xFFFF, (c & 0xFFFF00) | 0xFF, c}) {
            const auto it = klass.find(key);
            if (it == klass.end()) break;
            out += (out.empty() ? "" : " / ") + it->second;
        }
        return out.empty() ? std::string("(class not in pci.ids)") : out;
    }
};

static std::string driver_of(const fs::path& dev)
{
    std::error_code ec;
    const fs::path link = fs::read_symlink(dev / "driver", ec);
    return ec ? std::string("none") : link.filename().string();
}

int main()
{
    const PciIds ids("/usr/share/misc/pci.ids");
    std::cout << "pci.ids: " << ids.vendor.size() << " vendors, " << ids.device.size()
              << " devices, " << ids.klass.size() << " class entries\n\n";

    std::vector<fs::path> pci;
    for (const auto& e : fs::directory_iterator("/sys/bus/pci/devices")) pci.push_back(e.path());
    std::sort(pci.begin(), pci.end());
    std::cout << "== PCI functions (" << pci.size() << ") ==\n";
    for (const auto& d : pci) {
        const unsigned v = hexval(slurp(d / "vendor")), dv = hexval(slurp(d / "device"));
        const unsigned sv = hexval(slurp(d / "subsystem_vendor")), sd = hexval(slurp(d / "subsystem_device"));
        const unsigned cl = hexval(slurp(d / "class")), rev = hexval(slurp(d / "revision"));
        std::ostringstream o;
        o << std::hex;
        o.fill('0');
        o.width(4); o << v << ':'; o.width(4); o << dv << " sub "; o.width(4); o << sv << ':';
        o.width(4); o << sd << " rev "; o.width(2); o << rev << " class "; o.width(6); o << cl;
        std::cout << d.filename().string().substr(5) << "  " << o.str() << '\n'
                  << "        name:   " << ids.name_of(v, dv) << '\n'
                  << "        class:  " << ids.class_of(cl) << '\n'
                  << "        driver: " << driver_of(d) << '\n';
    }

    std::map<std::string, std::string> pnp;          // three-letter vendor prefix -> owner
    {
        std::ifstream f("/usr/share/hwdata/pnp.ids");
        std::string line;
        while (std::getline(f, line))
            if (line.size() > 4 && line[3] == '\t') pnp[line.substr(0, 3)] = line.substr(4);
    }
    std::vector<fs::path> acpi;
    for (const auto& e : fs::directory_iterator("/sys/bus/acpi/devices"))
        if (fs::exists(e.path() / "hid")) acpi.push_back(e.path());
    std::sort(acpi.begin(), acpi.end());
    std::cout << "\n== ACPI devices with a hardware ID (" << acpi.size() << ") ==\n";
    for (const auto& d : acpi) {
        const std::string hid = slurp(d / "hid");
        // Two ID shapes are checked: 3 letters + 4 hex digits (a PNP ID, vendor in pnp.ids) and
        // 4 letters or digits + 4 hex digits (an ACPI ID, vendor registry not available offline).
        auto hex4 = [&](std::size_t from) {
            return std::all_of(hid.begin() + static_cast<long>(from), hid.end(),
                               [](char ch) { return std::isxdigit(static_cast<unsigned char>(ch)) != 0; });
        };
        const bool pnp_shape = hid.size() == 7 && std::isalpha(static_cast<unsigned char>(hid[0])) &&
                               std::isalpha(static_cast<unsigned char>(hid[1])) &&
                               std::isalpha(static_cast<unsigned char>(hid[2])) && hex4(3);
        const bool acpi_shape = hid.size() == 8 && hex4(4);
        std::string vendor;
        if (pnp_shape) {
            const auto it = pnp.find(hid.substr(0, 3));
            vendor = "PNP ID, prefix " + hid.substr(0, 3) + ": " +
                     (it == pnp.end() ? std::string("(not in pnp.ids)") : it->second);
        } else if (acpi_shape) {
            vendor = "ACPI ID, vendor " + hid.substr(0, 4) + " (registry not available offline)";
        } else {
            vendor = "neither ID shape";
        }
        std::string where = slurp(d / "path");
        std::string bound = "none";
        for (const auto& e : fs::directory_iterator(d))     // the Linux device bound through it
            if (e.path().filename() == "physical_node") bound = fs::read_symlink(e.path()).filename().string();
        std::cout << hid;
        for (std::size_t i = hid.size(); i < 10; ++i) std::cout << ' ';
        std::cout << where << "  " << vendor << "  physical node: " << bound << '\n';
    }
    return 0;
}
