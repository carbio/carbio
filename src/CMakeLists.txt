add_library(carbio)

target_sources(carbio
PUBLIC
FILE_SET HEADERS
BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include
FILES
  ../include/carbio/fingerprint.h
  ../include/carbio/floating_point.h

PRIVATE
  fingerprint.cpp
)

target_include_directories(carbio
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
  $<INSTALL_INTERFACE:include>
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

find_package(Qt6 CONFIG REQUIRED COMPONENTS SerialPort)
target_link_libraries(carbio Qt6::SerialPort)

install(TARGETS carbio FILE_SET HEADERS)
