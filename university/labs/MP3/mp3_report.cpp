// MP3 Listing 4: turn raw benchmark records into the report table, refusing numbers that the
// measurement protocol (curriculum 13.4) does not support. Input: the targets file agreed at
// R1 followed by the raw records the harness wrote. Raw record kinds:
//   env gpu|driver|toolkit|clocks <text>          as printed by the tools on the GPU machine
//   check <kernel> <size> <precision> pass|fail   correctness result for that case
//   runs <kernel> own|ref <size> <precision> <timer> warmup <ms> times <ms> <ms> ...
// The verdict uses medians; the best (minimum) time is shown only to make the gap visible.
#include "mp3_targets.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>

struct Runs
{
    std::string timer;
    bool warmup = false;
    std::vector<double> ms;
};

double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

int main()
{
    const std::vector<Line> lines = parseRecords(std::cin);
    std::map<std::string, std::string> agreed, env, checks;
    std::map<std::string, Runs> runs;                  // key: kernel impl size precision
    for (const Line& l : lines) {
        if (l.kind == "gpu" || l.kind == "driver" || l.kind == "toolkit") {
            agreed[l.kind] = joined(l.words);
        } else if (l.kind == "env" && l.words.size() >= 2) {
            env[l.words[0]] = joined(l.words, 1);
        } else if (l.kind == "check" && l.words.size() >= 4) {
            checks[l.words[0] + " " + l.words[1] + " " + l.words[2]] = l.words[3];
        } else if (l.kind == "runs" && l.words.size() >= 6) {
            Runs& r = runs[l.words[0] + " " + l.words[1] + " " + l.words[2] + " " + l.words[3]];
            r.timer = l.words[4];
            for (std::size_t i = 5; i < l.words.size(); ++i) {
                if (l.words[i] == "warmup" && i + 1 < l.words.size()) {
                    r.warmup = true;
                    ++i;                               // the warm-up time is never used
                } else if (l.words[i] != "times") {
                    r.ms.push_back(std::atof(l.words[i].c_str()));
                }
            }
        }
    }
    bool sameMachine = true;
    for (const char* k : {"gpu", "driver", "toolkit"}) {
        std::printf("%-8s agreed at R1: %-28s measured: %s\n", k, agreed[k].c_str(),
                    env[k].c_str());
        sameMachine = sameMachine && !env[k].empty() && agreed[k] == env[k];
    }
    std::printf("clocks   %s\n", env["clocks"].empty() ? "(not recorded)" : env["clocks"].c_str());
    std::printf("fingerprint of the targets used: %s (must equal the one in the R1 gate record)\n",
                hex64(fingerprint(lines)).c_str());
    std::printf("%-9s %5s %-5s %4s %9s %9s %8s %8s %7s  %s\n", "kernel", "size", "prec", "runs",
                "own ms", "ref ms", "% median", "% best", "target", "verdict");
    int invalid = 0, met = 0, notMet = 0;
    for (const Line& t : lines) {
        if (t.kind != "target" || t.words.size() != 4) {
            continue;
        }
        const std::string kernel = t.words[0], size = t.words[1], prec = t.words[2];
        const double target = std::atof(t.words[3].c_str());
        const std::string tail = size + " " + prec;
        const auto own = runs.find(kernel + " own " + tail);
        const auto ref = runs.find(kernel + " ref " + tail);
        std::string why;
        if (own == runs.end() || ref == runs.end()) {
            why = "no data for own and reference";
        } else if (!sameMachine) {
            why = "measured on a machine other than the one agreed at R1";
        } else if (checks[kernel + " " + tail] != "pass") {
            why = "no passing correctness check for this case";
        } else if (own->second.timer != "events" || ref->second.timer != "events") {
            why = "timer is not device events for both";
        } else if (!own->second.warmup || !ref->second.warmup) {
            why = "no warm-up run recorded";
        } else if (own->second.ms.size() < 20 || ref->second.ms.size() < 20) {
            why = "fewer than 20 timed runs";
        }
        if (!why.empty()) {
            ++invalid;
            std::printf("%-9s %5s %-5s %4s %9s %9s %8s %8s %6.0f%%  INVALID: %s\n", kernel.c_str(),
                        size.c_str(), prec.c_str(), "-", "-", "-", "-", "-", target, why.c_str());
            continue;
        }
        const std::vector<double>& o = own->second.ms;
        const double mo = median(o), mr = median(ref->second.ms);
        const double bestO = *std::min_element(o.begin(), o.end());
        const double bestR = *std::min_element(ref->second.ms.begin(), ref->second.ms.end());
        const double pct = 100.0 * mr / mo, pctBest = 100.0 * bestR / bestO;
        const double spread = (*std::max_element(o.begin(), o.end()) - bestO) / mo;
        const bool ok = pct >= target;
        (ok ? met : notMet) += 1;
        std::printf("%-9s %5s %-5s %4zu %9.3f %9.3f %7.1f%% %7.1f%% %6.0f%%  %s%s\n",
                    kernel.c_str(), size.c_str(), prec.c_str(), o.size(), mo, mr, pct, pctBest,
                    target, ok ? "MET" : "NOT MET",
                    spread > 0.10 ? " (noisy: spread > 10 % of median)" : "");
    }
    std::printf("%d met, %d not met, %d invalid\n", met, notMet, invalid);
    return invalid == 0 ? 0 : 1;
}
