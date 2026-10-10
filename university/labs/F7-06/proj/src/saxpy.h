// saxpy.h - the one function the host code needs from the GPU part.
#pragma once
#include <vector>

// y = a * x + y on the GPU; returns an empty string or the first error message
const char* saxpyOnGpu(float a, const std::vector<float>& x, std::vector<float>& y);
