// consolidate.cpp - the worked example of F5-32: place virtual machines on hosts.
// Each host has physical CPUs (pCPUs) and memory; each VM asks for virtual CPUs (vCPUs) and
// memory. Rule set for this exercise: vCPUs may be overcommitted up to a ratio chosen by the
// operator, memory is never overcommitted, and a VM must fit inside ONE NUMA node of a host
// (so its memory is local, F5-28). First-fit decreasing by memory. All sizes are exercise
// values from consolidate.in, not properties of any real product.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Node
{
    int pcpus;
    int mem_gib;
    int used_vcpus = 0;
    int used_mem = 0;
};

struct Host
{
    std::string name;
    std::vector<Node> nodes;
};

struct Vm
{
    std::string name;
    int vcpus;
    int mem_gib;
};

int main()
{
    double ratio = 1.0;
    int hosts_n = 0;
    std::cin >> ratio >> hosts_n;
    std::vector<Host> hosts(static_cast<std::size_t>(hosts_n));
    for (Host& h : hosts) {
        int nodes = 0;
        std::cin >> h.name >> nodes;
        h.nodes.resize(static_cast<std::size_t>(nodes));
        for (Node& n : h.nodes) {
            std::cin >> n.pcpus >> n.mem_gib;
        }
    }
    std::vector<Vm> vms;
    Vm vm;
    while (std::cin >> vm.name >> vm.vcpus >> vm.mem_gib) {
        vms.push_back(vm);
    }
    std::stable_sort(vms.begin(), vms.end(), [](const Vm& x, const Vm& y) { return x.mem_gib > y.mem_gib; });
    std::cout << "vCPU overcommit ratio allowed: " << ratio << " vCPUs per pCPU\n";
    int unplaced = 0;
    for (const Vm& v : vms) {
        bool placed = false;
        for (Host& h : hosts) {
            for (std::size_t i = 0; i < h.nodes.size() && !placed; ++i) {
                Node& n = h.nodes[i];
                const bool cpu_ok = n.used_vcpus + v.vcpus <= static_cast<int>(n.pcpus * ratio);
                const bool mem_ok = n.used_mem + v.mem_gib <= n.mem_gib;
                if (cpu_ok && mem_ok) {
                    n.used_vcpus += v.vcpus;
                    n.used_mem += v.mem_gib;
                    std::cout << "  " << std::left << std::setw(6) << v.name << std::right
                              << std::setw(3) << v.vcpus << " vCPU " << std::setw(4) << v.mem_gib
                              << " GiB -> " << h.name << " node " << i << '\n';
                    placed = true;
                }
            }
            if (placed) {
                break;
            }
        }
        if (!placed) {
            std::cout << "  " << std::left << std::setw(6) << v.name << std::right << std::setw(3)
                      << v.vcpus << " vCPU " << std::setw(4) << v.mem_gib << " GiB -> NOT PLACED\n";
            ++unplaced;
        }
    }
    for (const Host& h : hosts) {
        for (std::size_t i = 0; i < h.nodes.size(); ++i) {
            const Node& n = h.nodes[i];
            std::cout << h.name << " node " << i << ": vCPUs " << n.used_vcpus << "/" << n.pcpus
                      << " pCPUs (" << std::fixed << std::setprecision(2)
                      << static_cast<double>(n.used_vcpus) / n.pcpus << "), memory " << n.used_mem
                      << "/" << n.mem_gib << " GiB\n";
        }
    }
    std::cout << "unplaced VMs: " << unplaced << '\n';
    return unplaced == 0 ? 0 : 3;
}
