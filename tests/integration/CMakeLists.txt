#----------------------------------------------------------------------
# Build rules

add_executable(integration_tests)

target_sources(integration_tests
  PRIVATE
  serial_port_tests.cpp
  test_main.cpp
)

target_include_directories(integration_tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(integration_tests
  PRIVATE
  GTest::gtest
  carbio
)

target_compile_options(integration_tests PRIVATE
  -Wno-unused-result
  -Wno-global-constructors
  -Wframe-larger-than=2048
)

#----------------------------------------------------------------------
# Test rules
include(GoogleTest)
gtest_discover_tests(integration_tests DISCOVERY_MODE PRE_TEST)

#----------------------------------------------------------------------