cmake_minimum_required(VERSION 3.16)
project(carbio)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Init.cmake)

add_subdirectory(src)
add_subdirectory(cli)

option(CARBIO_BUILD_EXAMPLES "Build examples" OFF)
if(CARBIO_BUILD_EXAMPLES)
add_subdirectory(examples)
endif()

option(CARBIO_BUILD_TESTS "Build unit tests" OFF)
if(CARBIO_BUILD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()
