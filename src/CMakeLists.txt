add_library(carbio)

target_sources(carbio
PUBLIC
FILE_SET HEADERS
BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include
FILES
  ../include/carbio/assert.h
  ../include/carbio/baud_rate_setting.h
  ../include/carbio/command_code.h
  ../include/carbio/command_executor.h
  ../include/carbio/command_serializer.h
  ../include/carbio/command_traits.h
  ../include/carbio/data_length_setting.h
  ../include/carbio/device_info.h
  ../include/carbio/device_setting_index.h
  ../include/carbio/device_setting_info.h
  ../include/carbio/endian.h
  ../include/carbio/fingerprint_sensor.h
  ../include/carbio/floating_point.h
  ../include/carbio/led_color_setting.h
  ../include/carbio/led_mode_setting.h
  ../include/carbio/locked_buffer.h
  ../include/carbio/match_query_info.h
  ../include/carbio/packet_id.h
  ../include/carbio/packet.h
  ../include/carbio/profile_store.h
  ../include/carbio/protocol_handler.h
  ../include/carbio/result.h
  ../include/carbio/search_query_info.h
  ../include/carbio/security_level_setting.h
  ../include/carbio/serial_port.h
  ../include/carbio/status_code.h

PRIVATE
  $<$<PLATFORM_ID:Linux>:serial_port_unix.cpp>
  $<$<PLATFORM_ID:Linux>:serial_port_unix.h>
  $<$<PLATFORM_ID:Linux>:descriptor_traits_unix.h>
  descriptor_traits.h
  descriptor.h
  device_info.cpp
  device_setting_info.cpp
  fingerprint_sensor.cpp
  match_query_info.cpp
  packet.cpp
  profile_store.cpp
  protocol_handler.cpp
  search_query_info.cpp
  serial_port.cpp
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

# OpenSSL for profile encryption
find_package(OpenSSL REQUIRED)
target_link_libraries(carbio OpenSSL::Crypto)

target_compile_features(carbio PUBLIC cxx_std_23)

#find_package(Qt6 CONFIG REQUIRED COMPONENTS SerialPort)
#target_link_libraries(carbio Qt6::SerialPort)

install(TARGETS carbio FILE_SET HEADERS)
