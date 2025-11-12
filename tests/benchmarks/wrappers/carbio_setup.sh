#!/usr/bin/env bash
#
# carbio Benchmark Setup Script
# Ensures templates are in place before benchmarks run
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CARBIO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# Find carbio CLI
CARBIO_CLI=""
POSSIBLE_PATHS=(
    "$CARBIO_ROOT/build/gcc-release-perf/bin/carbiok"
    "$CARBIO_ROOT/build/gcc-release-full-notest/bin/carbiok"
    "$CARBIO_ROOT/build/gcc-release-minimal-notest/bin/carbiok"
    "$CARBIO_ROOT/build/gcc-release/bin/carbiok"
    "$CARBIO_ROOT/build/Release/bin/carbiok"
)

for path in "${POSSIBLE_PATHS[@]}"; do
    if [[ -x "$path" ]]; then
        CARBIO_CLI="$path"
        break
    fi
done

if [[ -z "$CARBIO_CLI" ]]; then
    echo "Error: carbio CLI not found" >&2
    exit 1
fi

# Detect serial port
SERIAL_PORT="/dev/ttyAMA0"
[[ ! -e "$SERIAL_PORT" ]] && SERIAL_PORT="/dev/ttyUSB0"

echo "Setting up carbio benchmarks..."

# Ensure template at position 100 for verify/identify benchmarks
# Copy from reference position 149 (created during fingerprint capture)
echo "  - Copying reference template to position 100..."
"$CARBIO_CLI" --load --id 149 --buffer 1 --port "$SERIAL_PORT" > /dev/null 2>&1
"$CARBIO_CLI" --store --id 100 --buffer 1 --port "$SERIAL_PORT" > /dev/null 2>&1

echo "Setup complete!"
