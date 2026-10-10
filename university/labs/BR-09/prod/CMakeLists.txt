# SPDX-License-Identifier: LicenseRef-Uni-Lab
# stats: the SP202 course project (F2-26 template), made production-grade in BR-09.
# The version below is the single source of truth: stats_cli --version prints it and
# tools/check_release.py checks that CHANGELOG.md's newest entry has the same number.
cmake_minimum_required(VERSION 3.20)
project(uni_stats VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UNI_WERROR "Treat compiler warnings as errors" ON)
option(UNI_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(uni_flags INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(uni_flags INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    if(UNI_WERROR)
        target_compile_options(uni_flags INTERFACE -Werror)
    endif()
    if(UNI_SANITIZE)
        target_compile_options(uni_flags INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(uni_flags INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_library(stats src/stats.cpp src/parse.cpp)
target_include_directories(stats PUBLIC include)
target_link_libraries(stats PRIVATE uni_flags)

add_executable(stats_cli app/main.cpp)
target_compile_definitions(stats_cli PRIVATE UNI_VERSION="${PROJECT_VERSION}")
target_link_libraries(stats_cli PRIVATE stats uni_flags)

enable_testing()
foreach(t stats parse)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE stats uni_flags)
    add_test(NAME ${t}.unit COMMAND test_${t})
endforeach()

# CLI tests: exact exit code AND output (tests/expect.sh), happy path and failure paths.
set(EXPECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/expect.sh)
add_test(NAME cli.median COMMAND ${EXPECT} 0 "median 2$" $<TARGET_FILE:stats_cli> 3 1 2)
add_test(NAME cli.version COMMAND ${EXPECT} 0 "^stats_cli ${PROJECT_VERSION}$"
         $<TARGET_FILE:stats_cli> --version)
add_test(NAME cli.no_args COMMAND ${EXPECT} 2 "E-USAGE" $<TARGET_FILE:stats_cli>)
add_test(NAME cli.word COMMAND ${EXPECT} 3 "E-PARSE: argument 2 \"abc\" is not a number"
         $<TARGET_FILE:stats_cli> 1 abc)
add_test(NAME cli.trailing COMMAND ${EXPECT} 3 "has characters after" $<TARGET_FILE:stats_cli> 3abc)
add_test(NAME cli.huge COMMAND ${EXPECT} 3 "outside the range" $<TARGET_FILE:stats_cli> 1e999)
add_test(NAME cli.nan COMMAND ${EXPECT} 3 "not a finite" $<TARGET_FILE:stats_cli> nan 1 2)
add_test(NAME cli.overflow COMMAND ${EXPECT} 4 "E-RANGE.*KI-1" $<TARGET_FILE:stats_cli> 1e308 1e308)
# Compatibility: valid inputs give byte-identical output to version 0.1.
add_test(NAME cli.golden_v0_1 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden.sh
         $<TARGET_FILE:stats_cli> ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden_inputs.txt
         ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden_v0_1.txt)
