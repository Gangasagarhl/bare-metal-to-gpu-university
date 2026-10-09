// slo_model.hpp - a month of request counts for the F5-42 listings (included by slo.cpp,
// dashboard.cpp and budget.cpp). One entry per minute: requests and failed requests.
// Traffic follows a daily cycle; a small background error rate is always present; scripted
// incidents add errors. All rates are exercise values, not measurements of any service.
#pragma once
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

struct Minute
{
    long requests = 0;
    long errors = 0;
};

struct Incident
{
    std::string name;
    int start;      // minute of the month
    int minutes;    // duration
    double ratio;   // error ratio during the incident
};

constexpr int kMinutesPerDay = 1440;
constexpr int kDays = 30;
constexpr int kMonth = kDays * kMinutesPerDay;

inline int at(int day, int hour, int minute)   // day counts from 1
{
    return (day - 1) * kMinutesPerDay + hour * 60 + minute;
}

inline std::vector<Incident> labIncidents()
{
    return {
        {"hard outage of half the backends", at(4, 10, 0), 20, 0.50},
        {"total outage at night", at(12, 2, 0), 4, 1.00},
        {"bad deploy: slow burn", at(18, 0, 0), 3 * kMinutesPerDay, 0.003},
    };
}

// the month of the forensic lab "the budget is gone and nobody was paged" (budget.cpp)
inline std::vector<Incident> forensicIncidents()
{
    return {
        {"short spike of failures", at(9, 16, 20), 6, 0.25},
        {"deploy v2.31 until the rollback", at(14, 9, 12), at(27, 15, 40) - at(14, 9, 12), 0.0025},
    };
}

inline std::vector<Minute> generateMonth(const std::vector<Incident>& incidents, std::uint64_t seed)
{
    std::mt19937_64 g{seed};
    auto uniform = [&g]() { return (static_cast<double>(g() >> 11) + 0.5) * 0x1.0p-53; };
    const double pi = 3.14159265358979;
    std::vector<Minute> month(kMonth);
    for (int m = 0; m < kMonth; ++m) {
        const double hour = (m % kMinutesPerDay) / 60.0;
        // busiest at 15:00, quietest at 03:00
        const double mean = 2000.0 + 1500.0 * std::cos(2.0 * pi * (hour - 15.0) / 24.0);
        double ratio = 0.0002;   // background failures
        for (const auto& inc : incidents) {
            if (m >= inc.start && m < inc.start + inc.minutes) {
                ratio = inc.ratio;
            }
        }
        // binomial-like noise by a normal approximation (Box-Muller)
        const double z = std::sqrt(-2.0 * std::log(uniform())) * std::cos(2.0 * pi * uniform());
        const long req = std::lround(mean + std::sqrt(mean) * z);
        const double expErr = static_cast<double>(req) * ratio;
        const double z2 = std::sqrt(-2.0 * std::log(uniform())) * std::cos(2.0 * pi * uniform());
        long err = std::lround(expErr + std::sqrt(expErr * (1.0 - ratio)) * z2);
        err = err < 0 ? 0 : (err > req ? req : err);
        month[static_cast<std::size_t>(m)] = {req, err};
    }
    return month;
}

// prefix sums, so that the error ratio of any window costs two subtractions
struct Prefix
{
    std::vector<long> req;
    std::vector<long> err;
    explicit Prefix(const std::vector<Minute>& month) : req(month.size() + 1, 0), err(month.size() + 1, 0)
    {
        for (std::size_t m = 0; m < month.size(); ++m) {
            req[m + 1] = req[m] + month[m].requests;
            err[m + 1] = err[m] + month[m].errors;
        }
    }
    // error ratio over the window of `len` minutes that ends at minute m (inclusive)
    double ratio(int m, int len) const
    {
        const std::size_t hi = static_cast<std::size_t>(m) + 1;
        const std::size_t lo = m + 1 >= len ? static_cast<std::size_t>(m + 1 - len) : 0;
        const long r = req[hi] - req[lo];
        return r == 0 ? 0.0 : static_cast<double>(err[hi] - err[lo]) / static_cast<double>(r);
    }
};
