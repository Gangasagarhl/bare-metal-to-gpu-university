# The university template repository: one CMake project used by every later C++ lab.
cmake_minimum_required(VERSION 3.20)
project(uni_template VERSION 0.1.0 LANGUAGES CXX)

# One language standard for every target, with no compiler-specific dialect.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Write build/compile_commands.json so tools can see the exact compile commands.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UNI_WERROR "Treat compiler warnings as errors" ON)
option(UNI_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# An interface target that carries our warning and sanitizer flags to whoever links it.
add_library(uni_flags INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(uni_flags INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    if(UNI_WERROR)
        target_compile_options(uni_flags INTERFACE -Werror)
    endif()
    if(UNI_SANITIZE)
        target_compile_options(uni_flags INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(uni_flags INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# The code under test lives in a library, so the program and the tests share it.
add_library(stats src/stats.cpp)
target_include_directories(stats PUBLIC include)
target_link_libraries(stats PRIVATE uni_flags)

add_executable(stats_cli app/main.cpp)
target_link_libraries(stats_cli PRIVATE stats uni_flags)

# Tests: one unit-test program and one end-to-end run of the command-line tool.
enable_testing()
add_executable(test_stats tests/test_stats.cpp)
target_link_libraries(test_stats PRIVATE stats uni_flags)
add_test(NAME stats.unit COMMAND test_stats)
add_test(NAME stats.cli COMMAND stats_cli 3 1 2)
set_tests_properties(stats.cli PROPERTIES PASS_REGULAR_EXPRESSION "median 2")
