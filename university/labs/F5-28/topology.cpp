// topology.cpp - print the NUMA and socket layout that the Linux kernel of THIS machine exposes
// in sysfs: nodes, their CPUs, their memory and the node distance table, then each CPU's
// package (socket) and core number. It reads files only.
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string first_line(const fs::path& file)
{
    std::ifstream in(file);
    std::string line;
    if (!std::getline(in, line)) {
        return "(unreadable)";
    }
    return line;
}

std::vector<fs::path> numbered(const fs::path& dir, const std::string& prefix)
{
    std::vector<fs::path> out;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0 && name.size() > prefix.size() &&
            std::isdigit(static_cast<unsigned char>(name[prefix.size()]))) {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end(), [&](const fs::path& a, const fs::path& b) {
        return std::stoi(a.filename().string().substr(prefix.size())) <
               std::stoi(b.filename().string().substr(prefix.size()));
    });
    return out;
}

int main()
{
    const fs::path nodes_dir = "/sys/devices/system/node";
    const std::vector<fs::path> nodes = numbered(nodes_dir, "node");
    std::cout << "NUMA nodes seen by this kernel: " << nodes.size() << '\n';
    for (const fs::path& node : nodes) {
        std::cout << node.filename().string() << ": CPUs " << first_line(node / "cpulist")
                  << "; distances " << first_line(node / "distance") << '\n';
        std::cout << "  " << first_line(node / "meminfo") << '\n';
    }
    const std::vector<fs::path> cpus = numbered("/sys/devices/system/cpu", "cpu");
    std::cout << "CPUs: " << cpus.size() << '\n';
    for (const fs::path& cpu : cpus) {
        std::cout << "  " << cpu.filename().string()
                  << ": package (socket) " << first_line(cpu / "topology/physical_package_id")
                  << ", core " << first_line(cpu / "topology/core_id")
                  << ", threads sharing this core " << first_line(cpu / "topology/thread_siblings_list")
                  << '\n';
    }
    return nodes.empty() ? 1 : 0;
}
