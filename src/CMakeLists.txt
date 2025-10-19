add_library(carbio)

target_sources(carbio
PUBLIC
FILE_SET HEADERS
BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include
FILES
  ../include/carbio/core/assert.h
  ../include/carbio/fingerprint/baud_rate_setting.h
  ../include/carbio/fingerprint/command_code.h
  ../include/carbio/fingerprint/command_executor.h
  ../include/carbio/fingerprint/command_serializer.h
  ../include/carbio/fingerprint/command_traits.h
  ../include/carbio/fingerprint/device_info.h
  ../include/carbio/fingerprint/device_setting_index.h
  ../include/carbio/fingerprint/device_setting_info.h
  ../include/carbio/fingerprint/fingerprint_sensor.h
  ../include/carbio/fingerprint/led_color_setting.h
  ../include/carbio/fingerprint/led_mode_setting.h
  ../include/carbio/fingerprint/match_query_info.h
  ../include/carbio/fingerprint/packet_data_length_setting.h
  ../include/carbio/fingerprint/packet_id.h
  ../include/carbio/fingerprint/packet.h
  ../include/carbio/fingerprint/protocol_handler.h
  ../include/carbio/fingerprint/result.h
  ../include/carbio/fingerprint/search_query_info.h
  ../include/carbio/fingerprint/security_level_setting.h
  ../include/carbio/fingerprint/status_code.h
  ../include/carbio/io/handle.h
  ../include/carbio/io/handle_traits.h
  ../include/carbio/io/serial_port.h
  ../include/carbio/utility/blocking_queue.h
  ../include/carbio/utility/endian.h
  ../include/carbio/utility/floating_point.h
  ../include/carbio/utility/lockfree_triple_buffer.h
  ../include/carbio/utility/locked_buffer.h
  ../include/carbio/utility/spinlock.h

PRIVATE
  fingerprint/device_info.cpp
  fingerprint/device_setting_info.cpp
  fingerprint/fingerprint_sensor.cpp
  fingerprint/match_query_info.cpp
  fingerprint/packet.cpp
  fingerprint/protocol_handler.cpp
  fingerprint/search_query_info.cpp
  io/serial_port.cpp
)

target_include_directories(carbio
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
  $<INSTALL_INTERFACE:include>
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(carbio spdlog::spdlog_header_only)
install(TARGETS carbio FILE_SET HEADERS)
