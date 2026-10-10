// target_score.cpp - DR405 F4-45: rank candidate GPU-driver targets for one learner.
// Reads target_score.in (one target per line, five numbers at the end of the line) and
// scores each target. The weights are this chapter's teaching choice, not a standard:
// documentation and firmware dominate, an open driver to read and a machine to test on help,
// and a display-only target is a smaller first project.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Target {
    std::string name;
    int doc = 0, fw = 0, open = 0, owned = 0, display = 0, score = 0;
};

int main()
{
    std::vector<Target> ts;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        std::vector<std::string> words;
        for (std::string w; in >> w;) words.push_back(w);
        if (words.size() < 6) continue;
        Target t;
        const size_t k = words.size() - 5;
        for (size_t i = 0; i < k; ++i) t.name += (i ? " " : "") + words[i];
        t.doc = std::stoi(words[k]);
        t.fw = std::stoi(words[k + 1]);
        t.open = std::stoi(words[k + 2]);
        t.owned = std::stoi(words[k + 3]);
        t.display = std::stoi(words[k + 4]);
        t.score = 3 * t.doc - 3 * t.fw + 2 * t.open + 2 * t.owned + t.display;
        ts.push_back(t);
    }
    std::stable_sort(ts.begin(), ts.end(), [](const Target& a, const Target& b) { return a.score > b.score; });
    std::printf("%-34s %3s %3s %4s %5s %4s %6s  %s\n", "target", "doc", "fw", "open", "owned", "disp", "score", "verdict");
    for (const Target& t : ts) {
        const char* v = t.fw == 2 ? "out of reach for one person (firmware for everything)"
                        : t.doc == 0 ? "out of reach (no documentation)"
                        : t.score >= 12 ? "realistic first target"
                        : t.score >= 5 ? "realistic as a study or porting project"
                                       : "hard";
        std::printf("%-34s %3d %3d %4d %5d %4d %6d  %s\n", t.name.c_str(), t.doc, t.fw, t.open, t.owned, t.display,
                    t.score, v);
    }
    return 0;
}
