find_package(benchmark REQUIRED)

add_executable(fingerprint_benchmarks
  benchmark_main.cpp
  hardware_benchmarks.cpp
  scalability_database_benchmarks.cpp
  security_analysis_benchmarks.cpp
  workflow_hardware_benchmarks.cpp
)

target_include_directories(fingerprint_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(fingerprint_benchmarks
  PRIVATE
    carbio
    benchmark::benchmark
    benchmark::benchmark_main
)

add_test(
  NAME fingerprint_benchmarks
  COMMAND fingerprint_benchmarks --benchmark_min_time=0.1s --benchmark_filter=BM_Hardware_.*
)

set_tests_properties(fingerprint_benchmarks
  PROPERTIES
    TIMEOUT 600
    LABELS "performance;benchmark;hardware"
    RESOURCE_LOCK serial_port
)
