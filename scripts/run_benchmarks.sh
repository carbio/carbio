#!/bin/bash
###############################################################################
# Project   : Vehicle access control through biometric authentication
# Author    : Rajmund Kail
# Institute : Óbuda University
# Year      : 2025
#
# Performance Benchmark Runner Script
#
# This script runs all performance benchmarks and generates a comprehensive
# report for thesis evaluation.
###############################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${BUILD_DIR:-build/Release}"
RESULTS_DIR="${RESULTS_DIR:-benchmark_results}"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
BENCHMARK_ARGS="${BENCHMARK_ARGS:---benchmark_format=console --benchmark_out_format=json --benchmark_repetitions=5}"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}CARBIO Performance Benchmark Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory $BUILD_DIR not found${NC}"
    echo "Please build the project first with benchmarks enabled:"
    echo "  cmake --preset armv8-release -DCARBIO_BUILD_BENCHMARKS=ON"
    echo "  cmake --build build/Release"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# System information
echo -e "${GREEN}Collecting system information...${NC}"
{
    echo "# System Information"
    echo "Date: $(date)"
    echo "Hostname: $(hostname)"
    echo "Kernel: $(uname -r)"
    echo "Architecture: $(uname -m)"
    echo ""
    echo "## CPU Information"
    lscpu | grep -E "Model name|Architecture|CPU\(s\)|Thread|Core|Socket|MHz"
    echo ""
    echo "## Memory Information"
    free -h
    echo ""
    echo "## Raspberry Pi Model (if applicable)"
    if [ -f /proc/device-tree/model ]; then
        cat /proc/device-tree/model
        echo ""
    fi
} > "$RESULTS_DIR/system_info_${TIMESTAMP}.txt"

echo -e "${GREEN}System information saved to $RESULTS_DIR/system_info_${TIMESTAMP}.txt${NC}"
echo ""

# Function to run a benchmark
run_benchmark() {
    local name=$1
    local executable=$2
    local extra_args=${3:-}

    echo -e "${YELLOW}Running $name...${NC}"

    if [ ! -f "$BUILD_DIR/tests/benchmark/$executable" ]; then
        echo -e "${RED}  Error: Benchmark $executable not found${NC}"
        return 1
    fi

    local output_file="$RESULTS_DIR/${name}_${TIMESTAMP}"

    # Run benchmark with console output and JSON output
    "$BUILD_DIR/tests/benchmark/$executable" \
        $BENCHMARK_ARGS \
        --benchmark_out="${output_file}.json" \
        $extra_args \
        | tee "${output_file}.txt"

    echo -e "${GREEN}  Results saved to ${output_file}.json and ${output_file}.txt${NC}"
    echo ""

    return 0
}

# Run all benchmarks
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Running Benchmarks${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 1. Triple Buffer Benchmarks (most important - thesis innovation)
run_benchmark "triple_buffer" "triple_buffer_benchmark" "--benchmark_min_time=1.0"

# 2. Secure Value Benchmarks
run_benchmark "scoped_zero" "scoped_zero_benchmark" "--benchmark_min_time=1.0"

# 3. Memory Benchmarks
run_benchmark "memory" "memory_benchmark" "--benchmark_min_time=1.0"

# 4. Authentication Benchmarks (requires hardware)
echo -e "${YELLOW}Attempting authentication benchmarks (requires hardware)...${NC}"
if [ -z "$FINGERPRINT_PORT" ]; then
    echo -e "${YELLOW}  Note: FINGERPRINT_PORT not set, using default /dev/ttyAMA0${NC}"
    echo -e "${YELLOW}  Set FINGERPRINT_PORT environment variable if different${NC}"
fi

run_benchmark "authentication" "authentication_benchmark" "--benchmark_min_time=0.5" || {
    echo -e "${YELLOW}  Authentication benchmarks skipped (hardware not available)${NC}"
}

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Benchmark Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Generate summary report
SUMMARY_FILE="$RESULTS_DIR/summary_${TIMESTAMP}.md"

cat > "$SUMMARY_FILE" << EOF
# CARBIO Performance Benchmark Results

**Date:** $(date)
**System:** $(uname -m) $(uname -r)
**CPU:** $(lscpu | grep "Model name" | cut -d: -f2 | xargs)
**Memory:** $(free -h | grep Mem | awk '{print $2}')

## Benchmark Results

### Triple Buffer Performance

See: \`triple_buffer_${TIMESTAMP}.json\`

Key Metrics:
- Push latency (single-threaded)
- Pop latency (single-threaded)
- Producer-consumer throughput
- Multi-producer contention
- Comparison with mutex-based queue

### Secure Value Performance

See: \`scoped_zero_${TIMESTAMP}.json\`

Key Metrics:
- Secure value construction overhead
- Get/Set operation latency
- Volatile clear overhead
- Comparison with raw uint64_t

### Memory Usage

See: \`memory_${TIMESTAMP}.json\`

Key Metrics:
- Baseline memory footprint
- Component memory usage
- Memory leak detection
- Allocation performance
- Page fault analysis

### Authentication Performance

See: \`authentication_${TIMESTAMP}.json\`

Key Metrics:
- Capture image latency
- Extract features latency
- Search database latency
- Full authentication end-to-end latency
- Authentication throughput

## Analysis

### Latency Requirements (Automotive Standards)

| Operation | Target | Measured | Status |
|-----------|--------|----------|--------|
| Authentication (full) | < 500 ms | TBD | TBD |
| Sensor capture | < 100 ms | TBD | TBD |
| Database search | < 200 ms | TBD | TBD |

### Throughput Requirements

| Operation | Target | Measured | Status |
|-----------|--------|----------|--------|
| Authentication attempts/sec | > 2 | TBD | TBD |
| Lock-free buffer ops/sec | > 1M | TBD | TBD |

### Memory Requirements

| Component | Target | Measured | Status |
|-----------|--------|----------|--------|
| Total footprint | < 100 MB | TBD | TBD |
| Locked buffer overhead | < 10% | TBD | TBD |

## Conclusions

[To be filled in after analyzing benchmark results]

---
*Generated by CARBIO benchmark suite*
EOF

echo -e "${GREEN}Summary report created: $SUMMARY_FILE${NC}"
echo ""

# Optional: Generate graphs if Python is available
if command -v python3 &> /dev/null; then
    echo -e "${YELLOW}Generating performance graphs...${NC}"

    # Create a simple Python script to parse JSON and generate graphs
    cat > "$RESULTS_DIR/plot_results.py" << 'PYTHON_EOF'
#!/usr/bin/env python3
import json
import sys
import os
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("matplotlib not available, skipping graph generation")
    sys.exit(0)

def plot_benchmark_results(json_file):
    with open(json_file, 'r') as f:
        data = json.load(f)

    benchmarks = data.get('benchmarks', [])
    if not benchmarks:
        return

    names = [b['name'] for b in benchmarks]
    times = [b.get('cpu_time', 0) for b in benchmarks]

    # Create bar chart
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar(range(len(names)), times)
    ax.set_xticks(range(len(names)))
    ax.set_xticklabels(names, rotation=45, ha='right')
    ax.set_ylabel('Time (ns)')
    ax.set_title(f'Benchmark Results: {Path(json_file).stem}')
    plt.tight_layout()

    output = json_file.replace('.json', '.png')
    plt.savefig(output, dpi=150)
    print(f"Graph saved: {output}")

if __name__ == '__main__':
    results_dir = sys.argv[1] if len(sys.argv) > 1 else '.'
    for json_file in Path(results_dir).glob('*.json'):
        try:
            plot_benchmark_results(str(json_file))
        except Exception as e:
            print(f"Error plotting {json_file}: {e}")
PYTHON_EOF

    chmod +x "$RESULTS_DIR/plot_results.py"
    python3 "$RESULTS_DIR/plot_results.py" "$RESULTS_DIR" || true

    echo ""
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}All benchmarks complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Results saved to: $RESULTS_DIR"
echo "Summary report: $SUMMARY_FILE"
echo ""
echo "To analyze results:"
echo "  1. View JSON files for detailed metrics"
echo "  2. View .txt files for console output"
echo "  3. Review summary report: $SUMMARY_FILE"
echo ""
echo -e "${BLUE}For thesis documentation:${NC}"
echo "  - Include key latency measurements"
echo "  - Compare triple buffer vs mutex queue"
echo "  - Document memory footprint"
echo "  - Analyze authentication end-to-end latency"
echo ""
