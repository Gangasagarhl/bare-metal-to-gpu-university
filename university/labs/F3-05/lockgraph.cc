// lockgraph.cc - read a uni-rv lock trace and find who waits for whom.
// Lines look like:  cpu1 pid 4: acquire 'dir /home'   or   cpu0 pid 3: holds 'dir /home'
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> holder;   // lock name -> cpu holding it
    std::map<int, std::string> waiting;  // cpu -> lock it asked for but does not hold yet
    std::map<int, int> pidOf;
    std::string line;
    while (std::getline(std::cin, line)) {
        int cpu = -1, pid = -1;
        char verb[16] = {};
        if (std::sscanf(line.c_str(), "cpu%d pid %d: %15s", &cpu, &pid, verb) != 3) { continue; }
        const auto q1 = line.find('\'');
        const auto q2 = line.rfind('\'');
        if (q1 == std::string::npos || q2 == q1) { continue; }
        const std::string lock = line.substr(q1 + 1, q2 - q1 - 1);
        const std::string v = verb;
        pidOf[cpu] = pid;
        if (v == "acquire") { waiting[cpu] = lock; }
        if (v == "holds") {
            holder[lock] = cpu;
            waiting.erase(cpu);
        }
        if (v == "release") { holder.erase(lock); }
    }
    std::printf("locks held at the end of the trace:\n");
    for (const auto& [lock, cpu] : holder) {
        std::printf("  '%s' held by cpu%d (pid %d)\n", lock.c_str(), cpu, pidOf[cpu]);
    }
    std::printf("waits at the end of the trace (wait-for edges):\n");
    std::map<int, int> edge;
    for (const auto& [cpu, lock] : waiting) {
        const auto h = holder.find(lock);
        if (h == holder.end()) {
            std::printf("  cpu%d waits for '%s' (free: it should get it)\n", cpu, lock.c_str());
            continue;
        }
        edge[cpu] = h->second;
        std::printf("  cpu%d (pid %d) waits for '%s' -> held by cpu%d (pid %d)\n", cpu, pidOf[cpu],
                    lock.c_str(), h->second, pidOf[h->second]);
    }
    for (const auto& [start, unused] : edge) {
        int cur = start;
        std::string path = "cpu" + std::to_string(start);
        for (std::size_t steps = 0; steps <= edge.size(); ++steps) {
            const auto e = edge.find(cur);
            if (e == edge.end()) { break; }
            cur = e->second;
            path += " -> cpu" + std::to_string(cur);
            if (cur == start) {
                std::printf("CYCLE: %s  (deadlock: nobody in the cycle can ever continue)\n",
                            path.c_str());
                return 0;
            }
        }
    }
    std::printf("no cycle: no deadlock in this trace\n");
    return 0;
}
