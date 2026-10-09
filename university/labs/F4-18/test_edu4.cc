// test_edu4.cc - host unit and fault-injection tests for the F4-17 driver (edu4.cc, compiled
// unchanged for the host through the shims k4.h and pci4.h) against the observed-behaviour
// model edu_model.h.
//   test_edu4           the full suite
//   test_edu4 --weak    only the two weakest tests (used by the forensic lab)
#include "edu4.h"
#include "edu_model.h"
#include "shim_host.h"
#include <cstring>
#include <iostream>
#include <string>

struct ModelBackend : Backend {
    EduModel m;
    uint32_t read(uint32_t off) override { return m.read(off); }
    void write(uint32_t off, uint32_t v) override { m.write(off, v); }
};

static pci4::Function the_function()
{
    pci4::Function f{};
    f.vendor = 0x1234;
    f.device = 0x11E8;
    return f;
}

static int g_failed = 0, g_run = 0;
static void check(bool ok, const char* name, const std::string& detail = "")
{
    ++g_run;
    g_failed += !ok;
    std::cout << (ok ? "PASS  " : "FAIL  ") << name << (detail.empty() ? "" : "  (" + detail + ")") << '\n';
}

static edu4::Err probe_with(ModelBackend& b, edu4::Device& d)
{
    g_backend = &b;
    return edu4::probe(the_function(), d);
}

int main(int argc, char** argv)
{
    const bool weak = argc > 1 && std::strcmp(argv[1], "--weak") == 0;
    {   // T1 probe on a healthy device
        ModelBackend b;
        edu4::Device d;
        check(probe_with(b, d) == edu4::Err::Ok, "T1 probe succeeds on a healthy device");
    }
    if (weak) {   // the weak suite: "compute works" means "compute says Ok"
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        bool all_ok = true;
        for (uint32_t n = 0; n <= 14; ++n) {
            uint32_t r = 0;
            all_ok = all_ok && edu4::compute(d, n, r) == edu4::Err::Ok;
        }
        check(all_ok, "T4w compute returns Ok for n = 0..14");
        std::cout << g_run - g_failed << " of " << g_run << " tests passed\n";
        return g_failed ? 1 : 0;
    }
    {   // T2 an ID never observed
        ModelBackend b;
        b.m.id = 0x020000ED;
        edu4::Device d;
        check(probe_with(b, d) == edu4::Err::BadId, "T2 probe refuses an ID value never observed");
    }
    {   // T3 CHECK does not invert
        ModelBackend b;
        b.m.no_invert = true;
        edu4::Device d;
        check(probe_with(b, d) == edu4::Err::CheckFailed, "T3 probe refuses a device whose CHECK does not invert");
    }
    {   // T4 values
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        uint32_t expect = 1;
        int wrong = 0;
        for (uint32_t n = 0; n <= 14; ++n) {
            if (n) expect *= n;
            uint32_t r = 0;
            wrong += edu4::compute(d, n, r) != edu4::Err::Ok || r != expect;
        }
        check(wrong == 0, "T4 compute(n) == n! modulo 2^32 for n = 0..14", std::to_string(wrong) + " wrong");
    }
    {   // T5 stuck busy: must time out, and within its budget
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        b.m.stuck_busy = true;
        uint32_t r = 0;
        const uint64_t t0 = g_clock;
        const edu4::Err e = edu4::compute(d, 5, r);
        const uint64_t spent = g_clock - t0;
        check(e == edu4::Err::Timeout && spent <= d.timeout_ticks + 10000, "T5 a stuck device times out",
              std::string(edu4::to_string(e)) + ", " + std::to_string(spent) + " fake ticks");
    }
    {   // T6 surprise removal on real PCIe: all ones
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        b.m.all_ones = true;
        uint32_t r = 0;
        check(edu4::compute(d, 5, r) == edu4::Err::Gone, "T6 all-ones reads (removal) are reported as gone");
    }
    {   // T7 QEMU-style decoding off: zeros
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        b.m.zeros = true;
        uint32_t r = 0;
        check(edu4::compute(d, 5, r) == edu4::Err::Gone, "T7 all-zero reads (decoding off) are reported as gone");
    }
    {   // T8 interrupt status that never clears
        ModelBackend b;
        b.m.ack_ignored = true;
        edu4::Device d;
        check(probe_with(b, d) == edu4::Err::StuckIrq, "T8 probe reports an interrupt status that does not clear");
    }
    {   // T9 counters (observability)
        ModelBackend b;
        edu4::Device d;
        probe_with(b, d);
        uint32_t r = 0;
        for (int i = 0; i < 100; ++i) edu4::compute(d, 7, r);
        check(d.stats.computes == 100 && d.stats.polls == 400 && d.stats.timeouts == 0,
              "T9 counters: 100 computes, 4 STATUS polls each in this model",
              std::to_string(d.stats.computes) + " computes, " + std::to_string(d.stats.polls) + " polls");
    }
    std::cout << g_run - g_failed << " of " << g_run << " tests passed\n";
    return g_failed ? 1 : 0;
}
