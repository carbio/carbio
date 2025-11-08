cmake_minimum_required(VERSION 3.16)
project(carbio)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Init.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

option(CARBIO_BUILD_UNIT_TESTS "Build unit tests" OFF)
if(CARBIO_BUILD_UNIT_TESTS)
enable_testing()
add_subdirectory(tests/unit)
endif()

option(CARBIO_BUILD_INTEG_TESTS "Build integration tests" OFF)
if(CARBIO_BUILD_INTEG_TESTS)
enable_testing()
add_subdirectory(tests/integration)
endif()

option(CARBIO_BUILD_PERF_TESTS "Build performance tests" OFF)
if(CARBIO_BUILD_PERF_TESTS)
enable_testing()
add_subdirectory(tests/performance)
endif()