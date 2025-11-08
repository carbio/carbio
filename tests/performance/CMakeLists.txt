#----------------------------------------------------------------------
# Build fingerprint performance benchmarks with REAL HARDWARE
# Google Benchmark 1.9.4 provided by Conan (see conanfile.py)
#
# IMPORTANT: These benchmarks require a physical R30x sensor connected
# at /dev/ttyAMA0 (Raspberry Pi) or /dev/ttyUSB0 (USB adapter)
#----------------------------------------------------------------------

find_package(benchmark REQUIRED)

add_executable(fingerprint_benchmarks)

target_sources(fingerprint_benchmarks
  PRIVATE
  benchmark_main.cpp
  hardware_benchmarks.cpp
  scalability_database_benchmarks.cpp
  security_analysis_benchmarks.cpp
  workflow_hardware_benchmarks.cpp
)

target_include_directories(fingerprint_benchmarks
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against Google Benchmark and project libraries
# NOTE: GMock/GTest NOT needed - we test real hardware, not mocks
target_link_libraries(fingerprint_benchmarks
  PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main
  carbio
)

# Ensure optimizations are enabled for accurate benchmarks
target_compile_options(fingerprint_benchmarks
  PRIVATE
  -Wno-global-constructors
  -Wno-exit-time-destructors
)

#----------------------------------------------------------------------
# Benchmark execution targets
#----------------------------------------------------------------------

# Add custom target to run benchmarks with common options
add_custom_target(run_benchmarks
  COMMAND fingerprint_benchmarks
    --benchmark_format=console
    --benchmark_out=benchmark_results.json
    --benchmark_out_format=json
  DEPENDS fingerprint_benchmarks
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running fingerprint performance benchmarks..."
)

# Target to run benchmarks with repetitions for statistical analysis
add_custom_target(run_benchmarks_stats
  COMMAND fingerprint_benchmarks
    --benchmark_repetitions=10
    --benchmark_report_aggregates_only=true
    --benchmark_out=benchmark_stats.json
    --benchmark_out_format=json
  DEPENDS fingerprint_benchmarks
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running benchmarks with statistical analysis (10 repetitions)..."
)

# Target to run only workflow benchmarks (requires human interaction)
add_custom_target(run_workflow_benchmarks
  COMMAND fingerprint_benchmarks
    --benchmark_filter=BM_HardwareWorkflow.*
    --benchmark_out=workflow_results.json
    --benchmark_out_format=json
  DEPENDS fingerprint_benchmarks
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running hardware workflow benchmarks (requires human to place finger)..."
)

# Target to run command latency benchmarks (automated)
add_custom_target(run_hardware_benchmarks
  COMMAND fingerprint_benchmarks
    --benchmark_filter=BM_Hardware_.*
    --benchmark_out=hardware_results.json
    --benchmark_out_format=json
  DEPENDS fingerprint_benchmarks
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running hardware command latency benchmarks (automated)..."
)

#----------------------------------------------------------------------
# Register with CTest
# NOTE: Tests will skip gracefully if hardware is not available
#----------------------------------------------------------------------

add_test(
  NAME fingerprint_benchmarks
  COMMAND fingerprint_benchmarks
    --benchmark_min_time=0.1s
    --benchmark_filter=BM_Hardware_.*
)

# Set test properties
set_tests_properties(fingerprint_benchmarks
  PROPERTIES
  TIMEOUT 600  # Increased timeout for real hardware operations
  LABELS "performance;benchmark;hardware"
  ENVIRONMENT "GTEST_COLOR=1"  # Enable colored output
  RESOURCE_LOCK serial_port  # Prevent parallel execution on same hardware
)

# Note: Benchmarks requiring human interaction are NOT registered with CTest
# Run manually: cmake --build --preset=default --target run_workflow_benchmarks
