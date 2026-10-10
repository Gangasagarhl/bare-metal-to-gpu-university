// watchdog_test.cpp - F11-21: watchdog and safe-state tests (the SS402 lab).
// A window watchdog with a latched safe state, and a test suite for it. The same tests are
// then run against three deliberately wrong watchdogs ("mutants"); a good test suite must
// fail on each of them. Time is in milliseconds and is passed in by the tests (no clock).
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

struct Flaws {                    // switches that turn the correct watchdog into a mutant
    bool no_window = false;       // accepts kicks that come too early
    bool no_latch = false;        // a late kick clears the trip
    bool reset_ignores_estop = false;
};

class Watchdog {
public:
    Watchdog(int min_ms, int max_ms, Flaws f) : min_(min_ms), max_(max_ms), f_(f) {}

    void kick(int now)
    {
        if (tripped_ && !f_.no_latch) {
            return;                                  // a failed component cannot clear itself
        }
        if (now - last_ < min_ && !f_.no_window) {
            trip(now, "early kick");                 // a loop running too fast is also a fault
            return;
        }
        tripped_ = false;
        last_ = now;
    }

    void check(int now)                              // called by the monitor every period
    {
        if (!tripped_ && now - last_ > max_) {
            trip(now, "timeout");
        }
    }

    bool reset(int now, bool estop_pressed)          // operator reset
    {
        if (!tripped_ || (estop_pressed && !f_.reset_ignores_estop)) {
            return false;
        }
        tripped_ = false;
        last_ = now;
        command_ = 0;                                // motion restarts from zero
        return true;
    }

    bool drive_enabled() const { return !tripped_; }
    int command() const { return tripped_ ? 0 : command_; }
    void set_command(int c) { command_ = c; }
    int trip_time() const { return trip_time_; }
    const std::string& cause() const { return cause_; }

private:
    void trip(int now, const char* why)
    {
        tripped_ = true;
        trip_time_ = now;
        cause_ = why;
    }
    int min_, max_;
    Flaws f_;
    int last_ = 0;
    bool tripped_ = false;
    int trip_time_ = -1;
    int command_ = 0;
    std::string cause_;
};

using Test = std::function<bool(Flaws)>;

// T1: regular kicks inside the window never trip it
bool t1(Flaws f)
{
    Watchdog w(5, 50, f);
    for (int t = 10; t <= 1000; t += 10) {
        w.kick(t);
        w.check(t + 5);
    }
    return w.drive_enabled();
}

// T2: missing kicks trip it within max + check period (50 + 5 ms) and disable the drive
bool t2(Flaws f)
{
    Watchdog w(5, 50, f);
    w.set_command(300);
    w.kick(10);
    for (int t = 15; t <= 200; t += 5) {
        w.check(t);
    }
    return !w.drive_enabled() && w.command() == 0 && w.trip_time() <= 10 + 50 + 5;
}

// T3: a kick earlier than the window opens trips it
bool t3(Flaws f)
{
    Watchdog w(5, 50, f);
    w.kick(10);
    w.kick(12);
    return !w.drive_enabled() && w.cause() == "early kick";
}

// T4: the trip is latched: kicks after the trip do not re-enable the drive
bool t4(Flaws f)
{
    Watchdog w(5, 50, f);
    w.kick(10);
    w.check(100);
    w.kick(110);
    w.kick(120);
    return !w.drive_enabled();
}

// T5: reset is refused while the e-stop is pressed; accepted after release; restarts at 0
bool t5(Flaws f)
{
    Watchdog w(5, 50, f);
    w.set_command(300);
    w.kick(10);
    w.check(100);
    const bool refused = !w.reset(200, true);
    const bool accepted = w.reset(300, false);
    return refused && accepted && w.drive_enabled() && w.command() == 0;
}

int run_suite(const char* label, Flaws f, const std::vector<std::pair<std::string, Test>>& tests)
{
    int failed = 0;
    std::printf("%s\n", label);
    for (const auto& [name, test] : tests) {
        const bool ok = test(f);
        failed += ok ? 0 : 1;
        std::printf("  %-58s %s\n", name.c_str(), ok ? "PASS" : "FAIL");
    }
    return failed;
}

int main()
{
    const std::vector<std::pair<std::string, Test>> tests = {
        {"T1 regular kicks inside the window keep the drive enabled", t1},
        {"T2 missing kicks trip within 55 ms; drive off, command 0", t2},
        {"T3 an early kick trips the watchdog", t3},
        {"T4 the trip is latched; late kicks do not clear it", t4},
        {"T5 reset refused while e-stop pressed; restart from 0", t5},
    };
    const int real = run_suite("Watchdog under test (window 5..50 ms):", Flaws{}, tests);
    int survivors = 0;
    const std::vector<std::pair<const char*, Flaws>> mutants = {
        {"Mutant A: no early-kick window", Flaws{true, false, false}},
        {"Mutant B: not latched", Flaws{false, true, false}},
        {"Mutant C: reset ignores the e-stop", Flaws{false, false, true}},
    };
    for (const auto& [label, flaws] : mutants) {
        const int failed = run_suite(label, flaws, tests);
        std::printf("  -> %s\n", failed > 0 ? "mutant killed (the tests can see this flaw)"
                                            : "MUTANT SURVIVED (the tests miss this flaw)");
        survivors += failed > 0 ? 0 : 1;
    }
    std::printf("watchdog: %d of %zu tests failed; mutants surviving: %d\n", real, tests.size(),
                survivors);
    std::puts(real == 0 && survivors == 0 ? "ALL TESTS PASSED" : "TESTS FAILED");
    return (real == 0 && survivors == 0) ? 0 : 1;
}
