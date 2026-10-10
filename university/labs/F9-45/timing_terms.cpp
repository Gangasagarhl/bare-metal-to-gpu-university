// timing_terms.cpp - F9-45 Listing 1: compute the timing vocabulary of a periodic task
// (release, start latency, response time, slack, deadline miss, jitter) from a trace.
// The trace on stdin is a constructed teaching example (timing_terms.in), not a measurement.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Job {
    long release;   // when the job became due (microseconds)
    long start;     // when it started running
    long finish;    // when it finished
};

int main()
{
    long period = 0;
    long deadline = 0;           // relative deadline, measured from the release
    std::cin >> period >> deadline;
    std::vector<Job> jobs;
    Job j{};
    while (std::cin >> j.release >> j.start >> j.finish) {
        jobs.push_back(j);
    }
    if (jobs.size() < 2) {
        std::puts("need at least two jobs");
        return 1;
    }

    std::printf("period T = %ld us, relative deadline D = %ld us, %zu jobs\n",
                period, deadline, jobs.size());
    std::puts(" job  release  start-lat  response  slack   period(start-to-start)  verdict");

    std::vector<long> startLat, response, gaps;
    int misses = 0;
    for (std::size_t k = 0; k < jobs.size(); ++k) {
        const Job& x = jobs[k];
        const long lat = x.start - x.release;        // how late the job began
        const long resp = x.finish - x.release;      // how long from due to done
        const long slack = deadline - resp;          // negative slack = deadline missed
        startLat.push_back(lat);
        response.push_back(resp);
        long gap = 0;
        if (k > 0) {
            gap = x.start - jobs[k - 1].start;
            gaps.push_back(gap);
        }
        const bool miss = resp > deadline;
        misses += miss ? 1 : 0;
        std::printf("%4zu %8ld %10ld %9ld %6ld   %10s             %s\n", k, x.release, lat,
                    resp, slack, k > 0 ? std::to_string(gap).c_str() : "-",
                    miss ? "DEADLINE MISSED" : "ok");
    }

    auto [latMin, latMax] = std::minmax_element(startLat.begin(), startLat.end());
    auto [rMin, rMax] = std::minmax_element(response.begin(), response.end());
    auto [gMin, gMax] = std::minmax_element(gaps.begin(), gaps.end());
    std::puts("summary");
    std::printf("  start latency: min %ld, max %ld  -> release (start) jitter = %ld us\n",
                *latMin, *latMax, *latMax - *latMin);
    std::printf("  response time: min %ld, max %ld  -> response jitter = %ld us\n",
                *rMin, *rMax, *rMax - *rMin);
    std::printf("  start-to-start period: min %ld, max %ld (ideal %ld)"
                " -> worst period error = %ld us\n",
                *gMin, *gMax, period, std::max(period - *gMin, *gMax - period));
    std::printf("  worst-case response %ld us vs deadline %ld us; misses: %d of %zu\n",
                *rMax, deadline, misses, jobs.size());
    return 0;
}
