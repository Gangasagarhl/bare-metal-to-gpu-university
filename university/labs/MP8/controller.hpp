// controller.hpp - MP8 starter lab: the consumer side of C-grid v1 (the robot's controller).
// Deliberately small: the robot drives along +x on a straight line; the controller brakes
// when the inflated grid shows a lethal or unknown cell on the centre line ahead, and goes
// to a controlled stop (safe state, F9-69) when no usable frame is available.
#pragma once
#include "grid_msg.hpp"
#include <cmath>

namespace mp8 {

// True when a cell on the robot's centre line between x and x + lookahead (metres) is
// lethal or unknown. Inflation already added the robot's radius, so the centre line is enough.
inline bool pathBlocked(const GridMsg& m, double x, double y, double lookahead)
{
    const double cell = m.cellMm / 1000.0;
    const int j = static_cast<int>(std::floor(y / cell));
    const int i0 = static_cast<int>(std::floor(x / cell));
    const int i1 = static_cast<int>(std::floor((x + lookahead) / cell));
    for (int i = i0; i <= i1; ++i) {
        if (i < 0 || j < 0 || i >= m.width || j >= m.height) {
            return true;   // leaving the map is treated as blocked
        }
        const std::uint8_t c = m.cells[j * m.width + i];
        if (c == kLethal || c == kUnknown) {
            return true;
        }
    }
    return false;
}

}  // namespace mp8
