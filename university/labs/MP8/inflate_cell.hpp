// inflate_cell.hpp - MP8 starter lab: the perception kernel's per-cell rule, written once
// and compiled three times: by g++ for the CPU stand-in backend, by nvcc inside the CUDA
// kernel (inflate.cu) and by hipcc inside the HIP kernel (inflate.hip).
// Gather form: an output cell is lethal when ANY occupied input cell lies at one of the
// disc offsets around it. F9-54's rb::inflate() is the scatter form of the same rule and
// is the reference that contract_tests.cpp compares against.
#pragma once
#include <cstdint>

#if defined(__CUDACC__) || defined(__HIPCC__)
#define MP8_HD __host__ __device__
#else
#define MP8_HD
#endif

namespace mp8 {

// occ: width*height bytes, 1 = occupied, 0 = free (row-major).
// offs: nOffs pairs (di, dj) of the disc, built on the host from the inflation radius.
// Cells outside the map count as free here (contract sentence: "outside the map is not
// an obstacle of the map"; walls at the border are in the map itself).
MP8_HD inline std::uint8_t inflateCell(const std::uint8_t* occ, int width, int height,
                                       const int* offs, int nOffs, int i, int j)
{
    for (int k = 0; k < nOffs; ++k) {
        const int si = i - offs[2 * k];
        const int sj = j - offs[2 * k + 1];
        if (si >= 0 && sj >= 0 && si < width && sj < height && occ[sj * width + si] != 0) {
            return 100;   // kLethal of C-grid v1
        }
    }
    return 0;             // kFree
}

}  // namespace mp8
