# CMakeLists.txt - one source tree, two HIP back ends.
#   AMD (default):   cmake -S proj -B build -DCMAKE_CXX_COMPILER=clang++-17 -DGPU_TARGETS="gfx90a;gfx1030"
#   NVIDIA back end: cmake -S proj -B build -DHIP_BACKEND=NVIDIA -DCMAKE_CUDA_ARCHITECTURES=80
cmake_minimum_required(VERSION 3.21)
project(saxpy_hip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(HIP_BACKEND "AMD" CACHE STRING "HIP back end: AMD or NVIDIA")

add_library(kernels STATIC src/saxpy_kernel.hip)
if(HIP_BACKEND STREQUAL "NVIDIA")
    # HIP's NVIDIA back end is CUDA underneath: compile the .hip file as CUDA with nvcc.
    # HIP's headers select their NVIDIA implementation by themselves when nvcc compiles.
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set_source_files_properties(src/saxpy_kernel.hip PROPERTIES LANGUAGE CUDA)
else()
    # The hip package provides the imported target hip::device (compile and link flags
    # for GPU code); it reads the cache variable GPU_TARGETS for the --offload-arch list.
    find_package(hip REQUIRED)
    set_source_files_properties(src/saxpy_kernel.hip PROPERTIES LANGUAGE CXX)
    target_link_libraries(kernels PRIVATE hip::device)
endif()

add_executable(saxpy src/main.cpp)
target_link_libraries(saxpy PRIVATE kernels)
