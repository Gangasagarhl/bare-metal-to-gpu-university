// reqcheck.cpp - F12-11 Listing 2: is a performance requirement testable, and does it pass?
// Reads requirement lines "ID | text" from standard input. For each line it looks for the six
// parts this course asks every latency requirement to name (F12-11, Layer 2):
//   statistic (mean or pNN), threshold with unit, offered load, load-generator type,
//   measurement point, measurement window.
// It then runs the requirement against a SIMULATED service: one server, first come first
// served, service time exponential with mean 2 ms (an exercise model, not a real system).
// A requirement with missing parts is evaluated under two guesses, to show that its verdict
// depends on something nobody wrote down.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <vector>

struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
    double exponential(double mean) { return -mean * std::log(1.0 - uniform()); }
};

double const kServiceMs = 2.0;

// Latencies (ms) seen by the load generator during 'seconds' of simulated time.
// open loop: requests arrive at 'rate' per second whether or not earlier ones finished.
// closed loop: one client sends the next request only after the previous answer.
std::vector<double> simulate(double rate, double seconds, bool open_loop)
{
    Rng rng{2026};
    std::vector<double> lat;
    double const horizon_ms = seconds * 1000.0;
    if (!open_loop) {
        double now = 0.0;
        double const gap_ms = 1000.0 / rate;  // the client's intended pace
        while (now < horizon_ms) {
            double const service = rng.exponential(kServiceMs);
            lat.push_back(service);            // nobody else is ever in the queue
            now += std::max(gap_ms, service);  // waits for the answer before sending
        }
        return lat;
    }
    double arrival = 0.0;
    double server_free = 0.0;
    while (arrival < horizon_ms) {
        double const start = std::max(arrival, server_free);
        double const done = start + rng.exponential(kServiceMs);
        lat.push_back(done - arrival);
        server_free = done;
        arrival += rng.exponential(1000.0 / rate);
    }
    return lat;
}

double statistic(std::vector<double> v, std::string const& stat)
{
    if (stat == "mean") {
        double sum = 0.0;
        for (double x : v) {
            sum += x;
        }
        return sum / static_cast<double>(v.size());
    }
    double const p = std::stod(stat.substr(1));
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    rank = std::max<std::size_t>(rank, 1);
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

std::optional<std::string> find(std::string const& text, std::string const& pattern, int group)
{
    std::smatch m;
    if (std::regex_search(text, m, std::regex(pattern))) {
        return m[static_cast<std::size_t>(group)].str();
    }
    return std::nullopt;
}

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        auto const bar = line.find('|');
        std::string const id = line.substr(0, line.find(' '));
        std::string const text = line.substr(bar + 2);
        std::printf("%s: \"%s\"\n", id.c_str(), text.c_str());

        auto const stat = find(text, R"(\b(mean|p\d+(\.\d+)?)\b)", 1);
        auto const limit = find(text, R"(<=\s*([0-9.]+)\s*ms)", 1);
        auto const load = find(text, R"(at\s+([0-9.]+)\s*req/s)", 1);
        auto const loop = find(text, R"((open|closed)-loop)", 1);
        auto const point = find(text, R"(measured at (\w+))", 1);
        auto const window = find(text, R"(over\s+([0-9]+)\s*min)", 1);

        std::vector<std::string> missing;
        if (!stat) {
            missing.push_back("statistic");
        }
        if (!limit) {
            missing.push_back("threshold");
        }
        if (!load) {
            missing.push_back("offered load");
        }
        if (!loop) {
            missing.push_back("generator type");
        }
        if (!point) {
            missing.push_back("measurement point");
        }
        if (!window) {
            missing.push_back("window");
        }

        if (!stat || !limit) {
            std::printf("    NOT TESTABLE: no statistic or no threshold; nothing can fail it\n\n");
            continue;
        }
        double const threshold = std::stod(*limit);
        auto verdict = [&](double rate, bool open, double minutes) {
            double const value = statistic(simulate(rate, minutes * 60.0, open), *stat);
            std::printf("    %-5s at %5.0f req/s, %-11s: %7.2f ms  %s\n", stat->c_str(), rate,
                        open ? "open-loop" : "closed-loop", value,
                        value <= threshold ? "PASS" : "FAIL");
        };
        if (missing.empty()) {
            std::printf("    testable: all six parts present\n");
            verdict(std::stod(*load), *loop == "open", std::min(10.0, std::stod(*window)));
        } else {
            std::printf("    INCOMPLETE, missing:");
            for (auto const& m : missing) {
                std::printf(" [%s]", m.c_str());
            }
            std::printf("\n    the verdict depends on the guess:\n");
            verdict(load ? std::stod(*load) : 100.0, loop ? *loop == "open" : false, 10.0);
            verdict(load ? std::stod(*load) : 400.0, true, 10.0);
        }
        std::printf("\n");
    }
    return 0;
}
