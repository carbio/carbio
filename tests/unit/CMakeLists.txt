#----------------------------------------------------------------------
# Build rules

add_executable(unit_tests)

target_sources(unit_tests
  PRIVATE
  test_main.cpp
  serial_port_mock.h
  serial_port_tests.cpp
)

target_include_directories(unit_tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(unit_tests
  PRIVATE
  GTest::gtest
  GTest::gmock
  carbio
)

target_compile_options(unit_tests PRIVATE
  -Wno-global-constructors
  -Wframe-larger-than=4096
  -Wno-exit-time-destructors
)

#----------------------------------------------------------------------
# Test rules
include(GoogleTest)
gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)

#----------------------------------------------------------------------