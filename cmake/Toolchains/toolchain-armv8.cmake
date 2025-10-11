include_guard(GLOBAL)
set(CMAKE_CONFIGURATION_TYPES Debug Profile Release ASan TSan UBSan Rtsan)

# ------------------------------------------------------------------- #
# System specification
# ------------------------------------------------------------------- #

set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_PROCESSOR "aarch64")
set(CMAKE_SYSTEM_VERSION 1)

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")

# ------------------------------------------------------------------- #
# Compiler toolchain family
# ------------------------------------------------------------------- #

set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")
set(CMAKE_ASM_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_LINKER "aarch64-linux-gnu-ld")
set(CMAKE_LD "aarch64-linux-gnu-ld")
set(CMAKE_AS "aarch64-linux-gnu-as")
set(CMAKE_AR "aarch64-linux-gnu-ar")
set(CMAKE_NM "aarch64-linux-gnu-nm")
set(CMAKE_OBJCOPY "aarch64-linux-gnu-objcopy")
set(CMAKE_OBJDUMP "aarch64-linux-gnu-objdump")
set(CMAKE_READELF "aarch64-linux-gnu-readelf")
set(CMAKE_RANLIB "aarch64-linux-gnu-ranlib")
set(CMAKE_SIZE "aarch64-linux-gnu-size")
set(CMAKE_STRIP "aarch64-linux-gnu-strip")
set(CMAKE_STRINGS "aarch64-linux-gnu-strings")

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_ASM_COMPILER: ${CMAKE_ASM_COMPILER}")
message(STATUS "CMAKE_LINKER: ${CMAKE_LINKER}")
message(STATUS "CMAKE_LD: ${CMAKE_LD}")
message(STATUS "CMAKE_AS: ${CMAKE_AS}")
message(STATUS "CMAKE_AR: ${CMAKE_AR}")
message(STATUS "CMAKE_NM: ${CMAKE_NM}")
message(STATUS "CMAKE_OBJCOPY: ${CMAKE_OBJCOPY}")
message(STATUS "CMAKE_OBJDUMP: ${CMAKE_OBJDUMP}")
message(STATUS "CMAKE_READELF: ${CMAKE_READELF}")
message(STATUS "CMAKE_RANLIB: ${CMAKE_RANLIB}")
message(STATUS "CMAKE_SIZE: ${CMAKE_SIZE}")
message(STATUS "CMAKE_STRIP: ${CMAKE_STRIP}")
message(STATUS "CMAKE_STRINGS: ${CMAKE_STRINGS}")

# ------------------------------------------------------------------- #
# Language standards
# ------------------------------------------------------------------- #

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "CMAKE_C_STANDARD: ${CMAKE_C_STANDARD}")
message(STATUS "CMAKE_C_STANDARD_REQUIRED: ${CMAKE_C_STANDARD_REQUIRED}")
message(STATUS "CMAKE_C_EXTENSIONS: ${CMAKE_C_EXTENSIONS}")

message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_STANDARD_REQUIRED: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "CMAKE_CXX_EXTENSIONS: ${CMAKE_CXX_EXTENSIONS}")

# ------------------------------------------------------------------- #
# Compiler toolchain overrides
# ------------------------------------------------------------------- #

string(JOIN " " GCC_COMPILE_FLAGS_ARCH
  -mcpu=cortex-a76 -mtune=cortex-a76
  -march=armv8.2-a+crypto+fp16+rcpc+dotprod
)

string(JOIN " " GCC_COMPILE_FLAGS_BASE
  -Wall -Wextra -Wpedantic -Wuninitialized -Wmissing-include-dirs
  -Wshadow -Wundef -Winvalid-pch
  -Wfatal-errors
  -Werror
)

string(JOIN " " GCC_COMPILE_FLAGS_FLOW
  -Winit-self -Wswitch-enum -Wswitch-default
  -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k
  -Wduplicated-cond -Wduplicated-branches
)

string(JOIN " " GCC_COMPILE_FLAGS_ARITH
  -Wdouble-promotion -Wfloat-equal -Wpointer-arith
)

string(JOIN " " GCC_COMPILE_FLAGS_CAST
  -Wstrict-overflow=5 -Wcast-qual -Wcast-align -Wconversion -Wpacked
  -Wcast-align=strict
)

string(JOIN " " GCC_COMPILE_FLAGS_SAFETY
  -Wstrict-aliasing -fstrict-aliasing
  -Wredundant-decls -Wmissing-declarations -Wmissing-field-initializers
  -Wshift-overflow -Wshift-negative-value
  -Wnull-dereference -Wshift-overflow=2 
)

string(JOIN " " GCC_COMPILE_FLAGS_SECURITY
  -D_FORTIFY_SOURCE=3
  -Wwrite-strings -Wstack-protector -fstack-protector -fstack-clash-protection
  #-Wpadded
  -Winline -Wdisabled-optimization
  -Wlogical-op
  -Wstack-usage=1024 -fstack-usage -Wframe-larger-than=1024
  -Wtrampolines -Wvector-operation-performance
  -fsanitize=bounds -fsanitize-undefined-trap-on-error
  -Wunused-macros -Wstringop-overflow=4
  -Walloc-zero -Walloca
  -Wstringop-truncation
)

string(JOIN " " GCC_COMPILE_FLAGS_C
  -Waggregate-return -Wbad-function-cast -Wc++-compat
)

string(JOIN " " GCC_COMPILE_FLAGS_CPP
  -Wzero-as-null-pointer-constant -Wctor-dtor-privacy
  -Wold-style-cast -Woverloaded-virtual
  -Wuseless-cast -Wnoexcept -Wstrict-null-sentinel
  -Wsuggest-final-types -Wsuggest-final-methods
  -Wsuggest-override
  -Wvirtual-inheritance -Wmultiple-inheritance
  #-Wtemplates
  -Wextra-semi
)

string(JOIN " " GCC_COMPILE_FLAGS_EXTRA
  ${GCC_COMPILE_FLAGS_FLOW} ${GCC_COMPILE_FLAGS_ARITH}
  ${GCC_COMPILE_FLAGS_CAST} ${GCC_COMPILE_FLAGS_SAFETY}
  ${GCC_COMPILE_FLAGS_SECURITY}
)

string(JOIN " " CMAKE_C_FLAGS
  ${GCC_COMPILE_FLAGS_ARCH} ${GCC_COMPILE_FLAGS_BASE}
  ${GCC_COMPILE_FLAGS_EXTRA} ${GCC_COMPILE_FLAGS_C}
)
string(JOIN " " CMAKE_CXX_FLAGS
  ${GCC_COMPILE_FLAGS_ARCH} ${GCC_COMPILE_FLAGS_BASE}
  ${GCC_COMPILE_FLAGS_EXTRA} ${GCC_COMPILE_FLAGS_CPP}
)

# Debug
set(GCC_COMPILE_FLAGS_DEBUG "-DDEBUG -O0 -g")
set(CMAKE_C_FLAGS_DEBUG "${GCC_COMPILE_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "${GCC_COMPILE_FLAGS_DEBUG}")

# Profile
set(GCC_COMPILE_FLAGS_PROFILE "-DNDEBUG -O2 -fno-omit-frame-pointer -fdata-sections -ffunction-sections")
set(GCC_LINK_FLAGS_PROFILE "-Wl,-z,separate-code,-z,nodlopen,-z,noexecstack,-z,now,-z,relro,--gc-sections,--as-needed")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${GCC_COMPILE_FLAGS_PROFILE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${GCC_COMPILE_FLAGS_PROFILE}")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${GCC_LINK_FLAGS_PROFILE}")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${GCC_LINK_FLAGS_PROFILE}")
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${GCC_LINK_FLAGS_PROFILE}")

# Release
set(GCC_COMPILE_FLAGS_RELEASE "-DNDEBUG -O3 -s -fdata-sections -ffunction-sections")
set(GCC_LINK_FLAGS_RELEASE "-Wl,-z,separate-code,-z,nodlopen,-z,noexecstack,-z,now,-z,relro,--gc-sections,--as-needed,--discard-all,--strip-all")
set(CMAKE_C_FLAGS_RELEASE "${GCC_COMPILE_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "${GCC_COMPILE_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${GCC_LINK_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${GCC_LINK_FLAGS_RELEASE}")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${GCC_LINK_FLAGS_RELEASE}")

# Address Sanitizer configuration
string(JOIN " " CMAKE_C_FLAGS_ASAN "-DNDEBUG" "-D_ASAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize=pointer-compare" "-fsanitize=pointer-subtract" "-fsanitize=leak")
string(JOIN " " CMAKE_CXX_FLAGS_ASAN "-DNDEBUG" "-D_ASAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=address" "-fsanitize=pointer-compare" "-fsanitize=pointer-subtract" "-fsanitize=leak")
set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address -fsanitize=leak")
set(CMAKE_SHARED_LINKER_FLAGS_ASAN "-fsanitize=address -fsanitize=leak")
set(CMAKE_MODULE_LINKER_FLAGS_ASAN "-fsanitize=address -fsanitize=leak")

# Thread Sanitizer configuration
string(JOIN " " CMAKE_C_FLAGS_TSAN "-DNDEBUG" "-D_TSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=thread")
string(JOIN " " CMAKE_CXX_FLAGS_TSAN "-DNDEBUG" "-D_TSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=thread")
set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread")
set(CMAKE_SHARED_LINKER_FLAGS_TSAN "-fsanitize=thread")
set(CMAKE_MODULE_LINKER_FLAGS_TSAN "-fsanitize=thread")

# Undefined Behavior Sanitizer configuration
string(JOIN " " CMAKE_C_FLAGS_UBSAN "-DNDEBUG" "-D_UBSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=undefined" "-fsanitize=bounds-strict" "-fsanitize=float-divide-by-zero" "-fsanitize=float-cast-overflow")
string(JOIN " " CMAKE_CXX_FLAGS_UBSAN "-DNDEBUG" "-D_UBSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=undefined" "-fsanitize=bounds-strict" "-fsanitize=float-divide-by-zero" "-fsanitize=float-cast-overflow")
set(CMAKE_EXE_LINKER_FLAGS_UBSAN "-fsanitize=undefined")
set(CMAKE_SHARED_LINKER_FLAGS_UBSAN "-fsanitize=undefined")
set(CMAKE_MODULE_LINKER_FLAGS_UBSAN "-fsanitize=undefined")

# Real-Time santizing configuration
string(JOIN " " CMAKE_C_FLAGS_RTSAN "-DNDEBUG" "-D_UBSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=realtime")
string(JOIN " " CMAKE_CXX_FLAGS_RTSAN "-DNDEBUG" "-D_UBSAN" "-O1" "-g" "-fno-omit-frame-pointer" "-fsanitize=realtime")
set(CMAKE_EXE_LINKER_FLAGS_UBSAN "-fsanitize=realtime")
set(CMAKE_SHARED_LINKER_FLAGS_UBSAN "-fsanitize=realtime")
set(CMAKE_MODULE_LINKER_FLAGS_UBSAN "-fsanitize=realtime")

foreach (cmp IN ITEMS C CXX ASM)
message(STATUS "CMAKE_${cmp}_FLAGS: ${CMAKE_${cmp}_FLAGS}")
foreach (cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
message(STATUS "CMAKE_${cmp}_FLAGS_${cfg}: " "${CMAKE_${cmp}_FLAGS_${cfg}}")
endforeach ()
endforeach ()

foreach (ld IN ITEMS EXE SHARED MODULE STATIC)
message(STATUS "CMAKE_${ld}_LINKER_FLAGS: " "${CMAKE_${ld}_LINKER_FLAGS}")
foreach (cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
message(STATUS "CMAKE_${ld}_LINKER_FLAGS_${cfg}: " "${CMAKE_${ld}_LINKER_FLAGS_${cfg}}")
endforeach ()
endforeach ()

# ------------------------------------------------------------------- #

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