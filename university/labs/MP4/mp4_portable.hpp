// MP4 Listing 1: the one platform layer of the MP4 source tree.
// Every other file includes this header and never tests a platform macro itself.
//   AMD (hipcc):                       __HIPCC__ is defined, __HIP_PLATFORM_AMD__ is defined
//   NVIDIA (nvcc -x cu -D__HIP_PLATFORM_NVIDIA__): __CUDACC__ is defined
//   plain C++ (g++, the CPU emulator): neither; MP4_HD expands to nothing
#pragma once

#if defined(__HIPCC__) || defined(__CUDACC__)
#define MP4_HD __host__ __device__
#else
#define MP4_HD
#endif

#if defined(__HIP_PLATFORM_NVIDIA__)
#define MP4_PLATFORM "hip-nvidia"
#elif defined(__HIPCC__)
#define MP4_PLATFORM "hip-amd"
#else
#define MP4_PLATFORM "cpu-emulator"
#endif

// Host code asks this constant instead of testing the macro (one place knows the platform).
#if defined(__HIP_PLATFORM_NVIDIA__)
inline constexpr bool kNvidiaPlatform = true;
#else
inline constexpr bool kNvidiaPlatform = false;
#endif
