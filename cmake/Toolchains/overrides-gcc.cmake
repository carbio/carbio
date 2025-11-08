# ------------------------------------------------------------------- #
# Compiler toolchain overrides
# ------------------------------------------------------------------- #

#string(JOIN " " GCC_COMPILE_FLAGS_ARCH
#  -mcpu=cortex-a76 -mtune=cortex-a76
#  -march=armv8.2-a+crypto+fp16+rcpc+dotprod
#)

string(JOIN " " GCC_COMPILE_FLAGS_BASE
  -Wall -Wextra -Wpedantic -Wuninitialized -Wmissing-include-dirs
  -Wshadow -Wundef -Winvalid-pch
  #-Wfatal-errors
  #-Werror
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
  ${CMAKE_C_FLAGS} ${GCC_COMPILE_FLAGS_BASE}
  ${GCC_COMPILE_FLAGS_EXTRA} ${GCC_COMPILE_FLAGS_C}
)
string(JOIN " " CMAKE_CXX_FLAGS
  ${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS_BASE}
  ${GCC_COMPILE_FLAGS_EXTRA} ${GCC_COMPILE_FLAGS_CPP}
)

# Debug
set(GCC_COMPILE_FLAGS_DEBUG "-DDEBUG -O0 -g")
set(CMAKE_C_FLAGS_DEBUG "${GCC_COMPILE_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "${GCC_COMPILE_FLAGS_DEBUG}")

# Profile
set(GCC_COMPILE_FLAGS_PROFILE "-DNDEBUG -O2 -fno-omit-frame-pointer -fdata-sections -ffunction-sections")
set(GCC_LINK_FLAGS_PROFILE "-Wl,-z,separate-code,-z,nodlopen,-z,noexecstack,-z,now,-z,relro,--gc-sections,--as-needed")
set(CMAKE_C_FLAGS_PROFILE "${GCC_COMPILE_FLAGS_PROFILE}")
set(CMAKE_CXX_FLAGS_PROFILE "${GCC_COMPILE_FLAGS_PROFILE}")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${GCC_LINK_FLAGS_PROFILE}")
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "${GCC_LINK_FLAGS_PROFILE}")
set(CMAKE_MODULE_LINKER_FLAGS_PROFILE "${GCC_LINK_FLAGS_PROFILE}")

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

foreach (cmp IN ITEMS C CXX ASM)
message(STATUS "CMAKE_${cmp}_FLAGS: ${CMAKE_${cmp}_FLAGS}")
foreach (cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL ASAN TSAN UBSAN)
message(STATUS "CMAKE_${cmp}_FLAGS_${cfg}: " "${CMAKE_${cmp}_FLAGS_${cfg}}")
endforeach ()
endforeach ()

foreach (ld IN ITEMS EXE SHARED MODULE STATIC)
message(STATUS "CMAKE_${ld}_LINKER_FLAGS: " "${CMAKE_${ld}_LINKER_FLAGS}")
foreach (cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL ASAN TSAN UBSAN)
message(STATUS "CMAKE_${ld}_LINKER_FLAGS_${cfg}: " "${CMAKE_${ld}_LINKER_FLAGS_${cfg}}")
endforeach ()
endforeach ()

# ------------------------------------------------------------------- #
