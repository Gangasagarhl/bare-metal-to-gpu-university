// forensic_fixed.cpp - the fix of the forensic lab. (1) average() says what it does with an
// empty batch: it returns no value. (2) The empty batch is an explicit, permanent test case.
// (3) The random test prints its seed when it fails, and takes the seed from the command line,
// so any failure can be replayed exactly. Replays the same 400 CI run numbers.
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <random>
#include <vector>

namespace {

std::optional<double> average(const std::vector<double>& v)
{
    if (v.empty()) {
        return std::nullopt;  // no readings, no average
    }
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

bool test_average_empty()
{
    return !average({}).has_value();
}

bool test_average_batch(unsigned seed)
{
    std::mt19937 rng(seed);
    std::vector<double> batch(rng() % 100);
    double lo = 1e9, hi = -1e9;
    for (double& x : batch) {
        x = static_cast<double>(rng() % 5000) / 1000.0;
        lo = std::min(lo, x);
        hi = std::max(hi, x);
    }
    const auto avg = average(batch);
    const bool ok = batch.empty() ? !avg : (avg && *avg >= lo - 1e-12 && *avg <= hi + 1e-12);
    if (!ok) {
        std::printf("test_average_batch FAILED with seed %u (replay: ./forensic_fixed %u)\n", seed,
                    seed);
    }
    return ok;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc == 2) {  // replay one seed
        const auto seed = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
        return test_average_batch(seed) ? 0 : 1;
    }
    int failures = test_average_empty() ? 0 : 1;
    std::printf("test_average_empty: %s\n", failures == 0 ? "pass" : "FAILED");
    for (unsigned run = 1; run <= 400; ++run) {
        failures += test_average_batch(run) ? 0 : 1;
    }
    std::printf("test_average_batch, CI run numbers 1-400 as seeds: %d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
