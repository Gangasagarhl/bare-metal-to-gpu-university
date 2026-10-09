// pvclock_model.cpp - how a guest reads a paravirtual clock (KVM's kvmclock) safely.
// The hypervisor publishes, per virtual CPU, a small record in guest memory: a TSC value,
// the system time at that TSC value, and a scale (mul, shift) to convert TSC ticks to
// nanoseconds. It updates the record while the guest may be reading it, so the record has
// a version counter: odd while an update is in progress. This program replays updates and
// reads in a fixed order (no threads, so the output is the same on every run).
// The field names and the conversion follow KVM's documentation "KVM-specific MSRs" as
// remembered (not opened in this build): see the chapter's unverified box.
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace {
struct TimeInfo {                  // the published record
    std::uint32_t version = 0;
    std::uint64_t tsc_timestamp = 0;
    std::uint64_t system_time = 0; // nanoseconds at tsc_timestamp
    std::uint32_t mul = 0;         // ns = ((tsc delta shifted by `shift`) * mul) >> 32
    std::int8_t shift = 0;
};

// Scale so that ((delta << shift) * mul) >> 32 == delta * 1e9 / hz, with mul < 2^32.
void scale_for(std::uint64_t hz, std::uint32_t& mul, std::int8_t& shift)
{
    std::uint64_t num = 1000000000ull;
    std::uint64_t den = hz;
    shift = 0;
    while (den > num * 2) { den >>= 1; --shift; }        // fast clocks: shift right
    while (num >= den) { num >>= 1; ++shift; }            // keep the ratio below 1
    mul = static_cast<std::uint32_t>((num << 32) / den);
}

std::uint64_t to_ns(const TimeInfo& t, std::uint64_t tsc)
{
    std::uint64_t delta = tsc - t.tsc_timestamp;
    delta = t.shift >= 0 ? delta << t.shift : delta >> -t.shift;
    // (delta * mul) >> 32 without overflow: split delta into its high and low 32 bits.
    std::uint64_t scaled = (delta >> 32) * t.mul + (((delta & 0xffffffffull) * t.mul) >> 32);
    return t.system_time + scaled;
}

// The host's update, split in three steps so that a read can fall between them.
void update_begin(TimeInfo& t) { ++t.version; }                      // now odd
void update_fields(TimeInfo& t, std::uint64_t tsc, std::uint64_t ns) { t.tsc_timestamp = tsc; t.system_time = ns; }
void update_end(TimeInfo& t) { ++t.version; }                        // even again

// A careful reader copies the record and keeps the copy only if the version was even and
// unchanged. Here `snapshot` is what the guest saw at its first look.
bool careful_read(const TimeInfo& first_look, const TimeInfo& now, std::uint64_t tsc, std::uint64_t& ns)
{
    if ((first_look.version & 1) || first_look.version != now.version) {
        return false;                                    // retry
    }
    ns = to_ns(first_look, tsc);
    return true;
}
}

int main()
{
    const std::uint64_t hz = 2100000000ull;              // a 2.1 GHz TSC, as an example
    TimeInfo t;
    scale_for(hz, t.mul, t.shift);
    std::printf("scale for %llu Hz: mul %u, shift %d\n", static_cast<unsigned long long>(hz),
                t.mul, t.shift);
    // Check the scale against exact arithmetic for one second and one hour of ticks.
    for (std::uint64_t ticks : {hz, hz * 3600}) {
        TimeInfo z = t;
        std::uint64_t exact = (ticks / hz) * 1000000000ull + (ticks % hz) * 1000000000ull / hz;
        std::printf("  %llu ticks -> %llu ns (exact %llu)\n", static_cast<unsigned long long>(ticks),
                    static_cast<unsigned long long>(to_ns(z, ticks)), static_cast<unsigned long long>(exact));
    }
    // The record starts at TSC 0 = 0 ns; the host refreshes it at TSC 1,000,000,000
    // (= 476,190,476 ns at 2.1 GHz) while the guest is reading.
    update_begin(t); update_fields(t, 0, 0); update_end(t);
    const std::uint64_t tsc_a = 999000000;               // the guest reads just before ...
    std::uint64_t before = to_ns(t, tsc_a);
    update_begin(t);
    TimeInfo torn = t;                                   // a read in the middle of an update:
    torn.tsc_timestamp = 1000000000;                     // new timestamp already written,
                                                         // old system_time still there
    update_fields(t, 1000000000, 476190476);
    update_end(t);
    const std::uint64_t tsc_b = 1000500000;              // ... and just after the update
    std::printf("read at tsc %llu: %llu ns\n", static_cast<unsigned long long>(tsc_a),
                static_cast<unsigned long long>(before));
    std::printf("careless read during the update at tsc %llu: %llu ns  <- went backwards\n",
                static_cast<unsigned long long>(tsc_b), static_cast<unsigned long long>(to_ns(torn, tsc_b)));
    std::uint64_t ns = 0;
    bool ok = careful_read(torn, t, tsc_b, ns);
    std::printf("careful read, first look (version %u): %s\n", torn.version, ok ? "kept" : "rejected, retry");
    ok = careful_read(t, t, tsc_b, ns);
    std::printf("careful read, retry (version %u): %s, %llu ns\n", t.version, ok ? "kept" : "rejected",
                static_cast<unsigned long long>(ns));
    std::printf("monotonic: %s\n", ns >= before ? "yes" : "NO");
    return ns >= before ? 0 : 1;
}
