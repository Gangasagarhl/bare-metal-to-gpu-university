// forensic_history.cpp - forensic evidence: 400 CI runs of the same commit of one test.
// The test (as the team wrote it) draws a random batch of sensor readings and checks that
// their average lies between the smallest and the largest reading. Its random generator is
// seeded by the CI run number, which the test does not print. This program replays CI runs
// 1-400 and prints what CI showed: the failures, and the re-run that followed each failure.
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace {

double average(const std::vector<double>& v)  // the code under test
{
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

// The test, exactly as it ran in CI. Returns true if it passed; prints what it printed.
bool test_average_batch(unsigned ci_run_number)
{
    std::mt19937 rng(ci_run_number);
    std::vector<double> batch(rng() % 100);  // "a batch of up to 100 readings"
    double lo = 1e9, hi = -1e9;
    for (double& x : batch) {
        x = static_cast<double>(rng() % 5000) / 1000.0;  // a range in metres, 0 .. 4.999
        lo = std::min(lo, x);
        hi = std::max(hi, x);
    }
    const double avg = average(batch);
    if (!(avg >= lo - 1e-12 && avg <= hi + 1e-12)) {
        std::printf("    test_average_batch: FAILED: average %f not within the readings\n", avg);
        return false;
    }
    return true;
}

}  // namespace

int main()
{
    int failures = 0;
    for (unsigned run = 1; run <= 400; ++run) {
        if (!test_average_batch(run)) {
            ++failures;
            std::printf("CI run %3u: test_average_batch FAILED (commit unchanged)\n", run);
            const bool rerun = test_average_batch(run + 1000);  // CI gives a re-run a new number
            std::printf("CI run %3u: re-run of the same commit: %s\n", run + 1000,
                        rerun ? "passed" : "FAILED");
        }
    }
    std::printf("summary: 400 runs of the same commit, %d failed\n", failures);
    return 0;
}
