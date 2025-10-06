#----------------------------------------------------------------------
include_guard(GLOBAL)
# Specify search paths, i.e. let Cmake know where Conan places its third party stuff
set(USER_HOME_DIR "$ENV{HOME}")
list(APPEND CMAKE_PREFIX_PATH "${USER_HOME_DIR}/Qt/6.7.3/gcc_arm64")
#list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/build/generators")
list(APPEND CMAKE_MODULE_PATH "${USER_HOME_DIR}/Qt/6.7.3/gcc_arm64")
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build/generators")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
# Specify build paths, i.e. specify binary output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
# Use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# When building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(DEFINED CMAKE_INSTALL_RPATH)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${CMAKE_INSTALL_PREFIX}/lib")
else()
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()
message(STATUS "CMAKE_INSTALL_RPATH: ${CMAKE_INSTALL_RPATH}")
#----------------------------------------------------------------------