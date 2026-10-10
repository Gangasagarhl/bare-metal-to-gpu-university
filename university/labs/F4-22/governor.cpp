// governor.cpp - an idle governor as a model: given how long the CPU is predicted to stay idle,
// pick the deepest idle state whose target residency fits and whose exit latency the system
// accepts. The state table and the power numbers below are INVENTED TEACHING VALUES, not any
// processor's: a real table comes from the platform (ACPI _CST or the driver), see the chapter.
// The run compares three policies on the same sequence of idle periods:
//   always-poll, always-deepest, and the governor (prediction = average of the last 3 periods).
#include <cstdio>
#include <vector>

struct State { const char* name; double exit_latency_us, target_residency_us, power_mw; };
// Teaching table: deeper = less power while in it, longer to leave, worth it only if long enough.
static const std::vector<State> kStates = {
    {"POLL", 0, 0, 1000},
    {"C1 (halt)", 2, 2, 300},
    {"C3 (deeper)", 50, 150, 80},
    {"C6 (deepest)", 200, 800, 10},
};
constexpr double kWakeEnergyFactor = 1.0;   // energy to enter + leave = exit latency x poll power (model)

static int pick(double predicted_us, double latency_limit_us)
{
    int best = 0;
    for (int i = 0; i < static_cast<int>(kStates.size()); ++i)
        if (kStates[i].target_residency_us <= predicted_us && kStates[i].exit_latency_us <= latency_limit_us)
            best = i;
    return best;
}

struct Result { double energy_uj, late_us; int wakes_late; };
// Energy of one idle period of length t in state s, and how late the wake-up is.
static void spend(int s, double t, Result& r)
{
    const State& st = kStates[s];
    const double in_state = t > st.exit_latency_us ? t - st.exit_latency_us : 0;
    r.energy_uj += (in_state * st.power_mw + st.exit_latency_us * kStates[0].power_mw * kWakeEnergyFactor) / 1000.0;
    r.late_us += st.exit_latency_us;
    if (st.exit_latency_us > 0 && t < st.target_residency_us) ++r.wakes_late;   // went too deep
}

int main()
{
    // Idle periods in microseconds: a busy phase (short gaps), a quiet phase, then a mix.
    std::vector<double> periods;
    for (int i = 0; i < 20; ++i) periods.push_back(20 + (i % 3) * 5);
    for (int i = 0; i < 20; ++i) periods.push_back(5000);
    for (int i = 0; i < 20; ++i) periods.push_back(i % 2 ? 3000 : 100);
    const double latency_limit_us = 300;   // what the system can tolerate on a wake-up (model)

    Result poll{}, deep{}, gov{};
    int choice_count[4] = {};
    double h[3] = {20, 20, 20};
    for (size_t i = 0; i < periods.size(); ++i) {
        const double t = periods[i];
        spend(0, t, poll);
        spend(static_cast<int>(kStates.size()) - 1, t, deep);
        const double predicted = (h[0] + h[1] + h[2]) / 3;
        const int s = pick(predicted, latency_limit_us);
        ++choice_count[s];
        spend(s, t, gov);
        h[i % 3] = t;
        if (i % 10 == 0)
            std::printf("period %2zu: actual %6.0f us, predicted %6.0f us -> %s\n", i, t, predicted, kStates[s].name);
    }
    std::printf("governor choices:");
    for (size_t i = 0; i < kStates.size(); ++i) std::printf(" %s %d;", kStates[i].name, choice_count[i]);
    std::printf("\n%-15s %12s %14s %12s\n", "policy", "energy (uJ)", "added wake (us)", "too-deep");
    const struct { const char* n; Result r; } rows[] = {{"always-poll", poll}, {"always-deepest", deep}, {"governor", gov}};
    for (const auto& row : rows)
        std::printf("%-15s %12.1f %14.0f %12d\n", row.n, row.r.energy_uj, row.r.late_us, row.r.wakes_late);
    std::printf("(model units from the invented table above: compare the rows, not the numbers with any CPU)\n");
    return 0;
}
