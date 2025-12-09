cmake_minimum_required(VERSION 3.16)
project(carbio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable ccache automatically when available.
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

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

# Minimal CPack configuration so `cmake --build ... --target package` still works.
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)