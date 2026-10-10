// nvme_trace.cc - F1-50 Listing 2: read a QEMU NVMe trace (one event per line, as printed
// by QEMU's pci_nvme_* trace events) and summarise the queue traffic it shows:
// commands per queue, doorbell writes, completions, and every wrap of a doorbell value.
// Built and run by run.sh on the real trace of the firmware talking to QEMU's NVMe device.
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Value of "key <number>" or "key=<number>" inside a line (decimal or 0x hex).
static long field(const std::string& line, const std::string& key)
{
    std::size_t p = line.find(key + " ");
    std::size_t skip = key.size() + 1;
    if (p == std::string::npos) {
        p = line.find(key + "=");
    }
    if (p == std::string::npos) {
        return -1;
    }
    return std::stol(line.substr(p + skip), nullptr, 0);
}

static std::string quoted(const std::string& line)
{
    const std::size_t a = line.find('\'');
    const std::size_t b = line.rfind('\'');
    return (a == std::string::npos || b == a) ? "?" : line.substr(a + 1, b - a - 1);
}

int main()
{
    std::map<long, long> lastSqTail, lastCqHead, sqWraps, cqWraps, sqDoorbells, cqDoorbells;
    std::map<std::string, long> opcodes;
    long completions = 0, errors = 0, lines = 0;
    std::vector<std::string> firstCommands;
    std::string line;
    while (std::getline(std::cin, line)) {
        ++lines;
        std::istringstream in(line);
        std::string event;
        in >> event;
        if (event == "pci_nvme_mmio_doorbell_sq") {
            const long q = field(line, "sqid");
            const long t = field(line, "new_tail");
            if (lastSqTail.count(q) && t < lastSqTail[q]) {
                if (sqWraps[q]++ == 0) {
                    std::printf("SQ %ld tail wrapped: %ld -> %ld (so SQ %ld has %ld slots)\n",
                                q, lastSqTail[q], t, q, lastSqTail[q] + 1);
                }
            }
            lastSqTail[q] = t;
            ++sqDoorbells[q];
        } else if (event == "pci_nvme_mmio_doorbell_cq") {
            const long q = field(line, "cqid");
            const long h = field(line, "new_head");
            if (lastCqHead.count(q) && h < lastCqHead[q]) {
                if (cqWraps[q]++ == 0) {
                    std::printf("CQ %ld head wrapped: %ld -> %ld (so CQ %ld has %ld slots)\n",
                                q, lastCqHead[q], h, q, lastCqHead[q] + 1);
                }
            }
            lastCqHead[q] = h;
            ++cqDoorbells[q];
        } else if (event == "pci_nvme_admin_cmd" || event == "pci_nvme_io_cmd") {
            const std::string op = quoted(line);
            ++opcodes[op];
            if (firstCommands.size() < 6 || event == "pci_nvme_io_cmd") {
                std::ostringstream s;
                s << "sq " << field(line, "sqid") << " cid " << field(line, "cid") << " " << op;
                firstCommands.push_back(s.str());
            }
        } else if (event == "pci_nvme_enqueue_req_completion") {
            ++completions;
            if (field(line, "status") != 0) {
                ++errors;
            }
        }
    }
    std::printf("trace lines read: %ld\n", lines);
    for (const auto& [q, n] : sqDoorbells) {
        std::printf("SQ %ld: %ld tail doorbell writes, %ld wrap(s)\n", q, n, sqWraps[q]);
    }
    for (const auto& [q, n] : cqDoorbells) {
        std::printf("CQ %ld: %ld head doorbell writes, %ld wrap(s)\n", q, n, cqWraps[q]);
    }
    for (const auto& [op, n] : opcodes) {
        std::printf("command %-28s x %ld\n", op.c_str(), n);
    }
    std::printf("completions posted: %ld, with non-zero status: %ld\n", completions, errors);
    std::printf("first commands and every I/O command, in order:\n");
    for (const auto& c : firstCommands) {
        std::printf("  %s\n", c.c_str());
    }
    return 0;
}
