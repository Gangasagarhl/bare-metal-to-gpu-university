// contract_tests.cpp - MP8 starter lab: contract tests for C-grid v1, both sides.
// Producer side: the GPU-shaped gather rule equals the learner's F9-54 reference, and the
// frames it emits keep every sentence of the contract. Consumer side: the controller
// accepts exactly the frames the contract allows. Exit code 0 only if every test passes.
#include "controller.hpp"
#include "grid_msg.hpp"
#include "perception.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_run = 0;
int g_failed = 0;

void report(const std::string& name, bool ok, const std::string& detail = "")
{
    ++g_run;
    if (!ok) {
        ++g_failed;
    }
    std::printf("%-44s %s%s%s\n", name.c_str(), ok ? "PASS" : "FAIL",
                detail.empty() ? "" : "  ", detail.c_str());
}

int countDiff(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b)
{
    int d = 0;
    for (std::size_t k = 0; k < a.size(); ++k) {
        d += (a[k] != b[k]) ? 1 : 0;
    }
    return d;
}

int countLethal(const std::vector<std::uint8_t>& a)
{
    int n = 0;
    for (std::uint8_t c : a) {
        n += (c == mp8::kLethal) ? 1 : 0;
    }
    return n;
}

}  // namespace

int main()
{
    std::printf("== producer side (perception)\n");
    // The house of F9-52 with one box in the living room (metres).
    const rb::Grid house = rb::makeHouse({{3.0, 1.4, 3.3, 1.9}});
    const std::vector<std::uint8_t> occ = mp8::occBytes(house);
    for (double r : {0.15, 0.25, 0.35}) {
        const auto got = mp8::inflateGatherCpu(occ, r);
        const auto want = mp8::inflateReference(house, r);
        char name[64];
        std::snprintf(name, sizeof name, "P1 gather_equals_f954_reference r=%.2f", r);
        char detail[64];
        std::snprintf(detail, sizeof detail, "(%d lethal cells, %d differ)", countLethal(want), countDiff(got, want));
        report(name, got == want, detail);
    }
    {   // One obstacle cell in a corner of an otherwise empty map: the disc is clipped at the
        // border. This checks the contract sentence "outside the map is not an obstacle".
        rb::Grid corner;
        corner.set(0, 0, true);
        const auto got = mp8::inflateGatherCpu(mp8::occBytes(corner), 0.25);
        const auto want = mp8::inflateReference(corner, 0.25);
        char detail[64];
        std::snprintf(detail, sizeof detail, "(%d lethal cells, %d differ)", countLethal(want), countDiff(got, want));
        report("P2 corner_obstacle_clipped_like_reference", got == want, detail);
    }
    std::vector<mp8::GridMsg> frames;
    for (std::uint32_t s = 1; s <= 5; ++s) {
        frames.push_back(mp8::makeFrame(s, 100 * static_cast<std::int64_t>(s), mp8::inflateGatherCpu(occ, 0.25)));
    }
    bool valuesOk = true;
    bool geomOk = true;
    bool seqOk = true;
    for (std::size_t k = 0; k < frames.size(); ++k) {
        const mp8::GridMsg& f = frames[k];
        for (std::uint8_t c : f.cells) {
            valuesOk = valuesOk && (c == mp8::kFree || c == mp8::kLethal || c == mp8::kUnknown);
        }
        geomOk = geomOk && f.version == 1 && f.width == rb::kW && f.height == rb::kH && f.cellMm == 100 &&
                 f.cells.size() == static_cast<std::size_t>(f.width * f.height);
        seqOk = seqOk && (k == 0 || f.seq > frames[k - 1].seq);
    }
    report("P3 frame_cell_values_only_0_100_255", valuesOk);
    report("P4 frame_version_geometry_cell_mm", geomOk, "(80 x 60 cells, 100 mm)");
    report("P5 frame_seq_strictly_increasing", seqOk);

    std::printf("== consumer side (controller)\n");
    const mp8::Expect e{rb::kW, rb::kH, 100, 250};
    const mp8::GridMsg good = frames[2];          // seq 3, stamp 300 ms
    report("C1 accepts_valid_frame", mp8::check(good, e, 2, 400).empty());
    mp8::GridMsg v2 = good;
    v2.version = 2;
    std::string why = mp8::check(v2, e, 2, 400);
    report("C2 rejects_newer_version", !why.empty(), "(" + why + ")");
    mp8::GridMsg cm = good;
    cm.cellMm = 10;                               // a producer that "thinks in centimetres"
    why = mp8::check(cm, e, 2, 400);
    report("C3 rejects_other_cell_size", !why.empty(), "(" + why + ")");
    mp8::GridMsg cost = good;
    cost.cells[0] = 50;                           // a graded cost instead of 0/100/255
    why = mp8::check(cost, e, 2, 400);
    report("C4 rejects_unknown_cell_value", !why.empty(), "(" + why + ")");
    why = mp8::check(good, e, 3, 400);
    report("C5 rejects_repeated_seq", !why.empty(), "(" + why + ")");
    why = mp8::check(good, e, 2, 600);
    report("C6 rejects_stale_frame", !why.empty(), "(" + why + ")");
    why = mp8::check(good, e, 2, 250);
    report("C7 rejects_future_stamp", !why.empty(), "(" + why + ")");
    report("C8 accepts_seq_gap", mp8::check(frames[4], e, 2, 520).empty(), "(seq 2 -> 5)");
    mp8::GridMsg unk = mp8::makeFrame(9, 900, std::vector<std::uint8_t>(rb::kW * rb::kH, mp8::kFree));
    unk.cells[16 * rb::kW + 12] = mp8::kUnknown;  // one unknown cell ahead on row j = 16
    report("C9 unknown_cell_ahead_blocks", mp8::pathBlocked(unk, 0.85, 1.65, 0.5) &&
                                               !mp8::pathBlocked(unk, 1.35, 1.65, 0.5));

    std::printf("contract tests: %d run, %d failed\n", g_run, g_failed);
    return g_failed == 0 ? 0 : 1;
}
