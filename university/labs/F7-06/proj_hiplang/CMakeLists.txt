# CMakeLists.txt - the same program with CMake's own HIP language support.
cmake_minimum_required(VERSION 3.21)
project(saxpy_hiplang LANGUAGES CXX HIP)
set(CMAKE_HIP_ARCHITECTURES gfx90a)
add_executable(saxpy ../proj/src/main.cpp ../proj/src/saxpy_kernel.hip)
