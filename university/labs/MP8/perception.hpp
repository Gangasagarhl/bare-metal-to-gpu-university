// perception.hpp - MP8 starter lab: the host side of the perception component.
// Reuses the learner's RB401 code: the simulated house of F9-52 (house.hpp) and the
// reference inflation of F9-54 (planner.hpp). The CPU "gather" backend runs exactly the
// per-cell rule that the GPU kernels run (inflate_cell.hpp); it is the stub that stands
// behind contract C-grid in the walking skeleton until a GPU is available.
#pragma once
#include "../F9-54/planner.hpp"
#include "grid_msg.hpp"
#include "inflate_cell.hpp"
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace mp8 {

// Disc offsets with F9-54's own criterion (hypot(a, b) * kCell <= radius), so the
// gather form and the scatter reference agree by construction of the same disc.
inline std::vector<int> discOffsets(double radius)
{
    std::vector<int> offs;
    const int r = static_cast<int>(std::ceil(radius / rb::kCell));
    for (int b = -r; b <= r; ++b) {
        for (int a = -r; a <= r; ++a) {
            if (std::hypot(a, b) * rb::kCell <= radius) {
                offs.push_back(a);
                offs.push_back(b);
            }
        }
    }
    return offs;
}

// Occupancy bytes of an rb::Grid (inside cells only).
inline std::vector<std::uint8_t> occBytes(const rb::Grid& g)
{
    std::vector<std::uint8_t> occ(rb::kW * rb::kH, 0);
    for (int j = 0; j < rb::kH; ++j) {
        for (int i = 0; i < rb::kW; ++i) {
            occ[j * rb::kW + i] = g.occ(i, j) ? 1 : 0;
        }
    }
    return occ;
}

// CPU gather backend: the same loop body as the GPU kernels, one cell at a time.
inline std::vector<std::uint8_t> inflateGatherCpu(const std::vector<std::uint8_t>& occ, double radius)
{
    const std::vector<int> offs = discOffsets(radius);
    const int nOffs = static_cast<int>(offs.size() / 2);
    std::vector<std::uint8_t> out(occ.size(), 0);
    for (int j = 0; j < rb::kH; ++j) {
        for (int i = 0; i < rb::kW; ++i) {
            out[j * rb::kW + i] = inflateCell(occ.data(), rb::kW, rb::kH, offs.data(), nOffs, i, j);
        }
    }
    return out;
}

// The learner's F9-54 reference (scatter), converted to C-grid cell values.
inline std::vector<std::uint8_t> inflateReference(const rb::Grid& g, double radius)
{
    const rb::CostGrid c = rb::inflate(g, radius);
    std::vector<std::uint8_t> out(rb::kW * rb::kH, 0);
    for (int k = 0; k < rb::kW * rb::kH; ++k) {
        out[k] = (c.cost[k] == rb::kLethal) ? kLethal : kFree;
    }
    return out;
}

// Producer: one C-grid v1 frame.
inline GridMsg makeFrame(std::uint32_t seq, std::int64_t stampMs, std::vector<std::uint8_t> cells)
{
    GridMsg m;
    m.seq = seq;
    m.stampMs = stampMs;
    m.width = rb::kW;
    m.height = rb::kH;
    m.cellMm = static_cast<int>(std::lround(rb::kCell * 1000.0));
    m.cells = std::move(cells);
    return m;
}

}  // namespace mp8
