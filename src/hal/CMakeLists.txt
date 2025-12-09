# Build target
add_library(carbio)

target_sources(carbio
PUBLIC
FILE_SET HEADERS
BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
FILES
  core/assert.h
  fingerprint/command_code.h
  fingerprint/command_deserializer.h
  fingerprint/command_executor.h
  fingerprint/command_serializer.h
  fingerprint/command_traits.h
  fingerprint/device_info.h
  fingerprint/device_settings.h
  fingerprint/fingerprint_sensor.h
  fingerprint/match_query_info.h
  fingerprint/packet.h
  fingerprint/protocol_handler.h
  fingerprint/result.h
  fingerprint/search_query_info.h
  fingerprint/sensor_types.h
  fingerprint/user_data.h
  io/handle.h
  io/handle_traits.h
  io/serial_port.h
  utility/conversion.h
  utility/endian.h
  utility/floating_point.h
  utility/locked_buffer.h
  utility/scoped_zero.h
  utility/spinlock.h

PRIVATE
  fingerprint/device_info.cpp
  fingerprint/device_settings.cpp
  fingerprint/fingerprint_sensor.cpp
  fingerprint/match_query_info.cpp
  fingerprint/packet.cpp
  fingerprint/protocol_handler.cpp
  fingerprint/search_query_info.cpp
  fingerprint/user_data.cpp
  io/serial_port.cpp
)

target_include_directories(carbio
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<INSTALL_INTERFACE:include>
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(carbio spdlog::spdlog_header_only)


# Install target
install(TARGETS carbio FILE_SET HEADERS)
