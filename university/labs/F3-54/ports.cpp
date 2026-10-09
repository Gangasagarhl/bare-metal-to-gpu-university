// ports.cpp - F3-54: the planning half of a ports system. It reads recipe headers
// ("name version: dependencies...") and prints a build order in waves: every recipe in a wave
// depends only on recipes of earlier waves, so a wave's recipes could be built in parallel.
// Missing dependencies and dependency cycles are reported instead of being built around.
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Recipe
{
    std::string version;
    std::set<std::string> deps;
};

int main()
{
    std::map<std::string, Recipe> recipes;           // std::map: sorted, so output is stable
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        std::string name, version, dep;
        in >> name >> version;
        if (!version.empty() && version.back() == ':') version.pop_back();
        Recipe r {version, {}};
        while (in >> dep) r.deps.insert(dep);
        recipes[name] = r;
    }

    // A recipe is blocked if a dependency has no recipe or is itself blocked.
    std::set<std::string> blocked;
    for (const auto& [name, r] : recipes) {
        for (const auto& d : r.deps) {
            if (recipes.count(d) == 0) {
                std::cout << "error: " << name << " needs " << d << ", which has no recipe\n";
                blocked.insert(name);
            }
        }
    }
    for (bool grew = true; grew;) {
        grew = false;
        for (const auto& [name, r] : recipes) {
            for (const auto& d : r.deps) {
                if (blocked.count(d) != 0 && blocked.insert(name).second) grew = true;
            }
        }
    }
    for (const auto& b : blocked) recipes.erase(b);
    if (!blocked.empty()) {
        std::cout << "not built (blocked by the errors above):";
        for (const auto& b : blocked) std::cout << ' ' << b;
        std::cout << '\n';
    }

    std::set<std::string> built;
    int wave = 0;
    while (built.size() < recipes.size()) {
        std::vector<std::string> ready;
        for (const auto& [name, r] : recipes) {
            if (built.count(name) != 0) continue;
            bool all = true;
            for (const auto& d : r.deps) all = all && built.count(d) != 0;
            if (all) ready.push_back(name);
        }
        if (ready.empty()) {
            std::cout << "error: dependency cycle among:";
            for (const auto& [name, r] : recipes) {
                if (built.count(name) == 0) std::cout << ' ' << name;
            }
            std::cout << '\n';
            break;
        }
        std::cout << "wave " << ++wave << ':';
        for (const auto& n : ready) {
            std::cout << ' ' << n << '-' << recipes[n].version;
            built.insert(n);
        }
        std::cout << '\n';
    }
    std::cout << built.size() << " recipes planned in " << wave << " waves\n";
    return built.size() == recipes.size() && blocked.empty() ? 0 : 1;
}
