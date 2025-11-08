include_guard(GLOBAL)

# ------------------------------------------------------------------- #
# Toolchain configuration
# ------------------------------------------------------------------- #

set(CMAKE_CONFIGURATION_TYPES Debug Profile Release ASan TSan UBSan)

set(CMAKE_C_COMPILER   gcc)
set(CMAKE_CXX_COMPILER g++)

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

# ------------------------------------------------------------------- #
# Build flag setup
# ------------------------------------------------------------------- #

include(${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake)

# ------------------------------------------------------------------- #
# Parallel build
# ------------------------------------------------------------------- #

# Boost compilation speed

set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 4)
#
# Figure out the CPU count and parallel execution flags.  For a build of any
# given sub-project you can add "-- -j <n>" to the end of the build command to
# perform parallel builds.  This is an attempt to get the superbuild
# orchestration script to properly pass the parallel execution flags to all
# sub-builds.
#
include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(NOT CPU_COUNT EQUAL 0)
  if(CMAKE_GENERATOR STREQUAL "Ninja")
    # Try not to kill older Raspberry Pis
    execute_process(COMMAND cat /proc/device-tree/model OUTPUT_VARIABLE MODEL ERROR_QUIET)
    if(MODEL MATCHES "Raspberry Pi (.)")
      if(${CMAKE_MATCH_1} LESS 4)
        set(CPU_COUNT 2)
      endif()
    endif()
    message(STATUS "CPU_COUNT: ${CPU_COUNT}")
    # Building with ninja, 'cmake --build build -j <n>' works properly for cmake
    # based projects, but not for make based projects.  Use this variable to
    # pass the cpu count into make based projects.
    set(MAKE_JFLAG -j${CPU_COUNT})
    # Tell ninja that it can execute as many parallel compile jobs as there are
    # CPUs, but it can only execute one link job at a time. This is probably
    # only necessary for small memory systems, but set it for all systems. Make
    # this a cache variable so that the user can override it from the command
    # line.
    set(CMAKE_JOB_POOLS "compile=${CPU_COUNT};link=1" CACHE STRING "Ninja job pools for parallel builds")
    set(CMAKE_JOB_POOL_COMPILE "compile" CACHE STRING "Job pool for compilation")
    set(CMAKE_JOB_POOL_LINK    "link" CACHE STRING "Job pool for linking")
    # Tell ninja that it can execute as many parallel jobs as there are CPUs.
    set(CTEST_BUILD_FLAGS -j${CPU_COUNT})
    set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${CPU_COUNT})
  else()
    # Building with unix makefiles, 'cmake --build build -j <n>' works properly.
  endif()
endif()