#!/usr/bin/env bash
#
# carbio Library Wrapper for Hyperfine Benchmarking
# Provides consistent CLI interface for benchmarking operations
#

set -euo pipefail

OPERATION="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FP_DATA="${SCRIPT_DIR}/../fingerprint_data/captured_template.bin"
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

case "$OPERATION" in
    store)
        # Copy template: load from reference position 149 -> store to benchmark position 100
        # We use position 149 as the reference template (captured during benchmark setup)
        "$CARBIO_CLI" --load --id 149 --buffer 1 --port "$SERIAL_PORT" > /dev/null 2>&1
        exec "$CARBIO_CLI" --store --id 100 --buffer 1 --port "$SERIAL_PORT"
        ;;

    load)
        # Load template from sensor to buffer
        exec "$CARBIO_CLI" --load --id 100 --buffer 1 --port "$SERIAL_PORT"
        ;;

    delete)
        # Delete template from sensor
        exec "$CARBIO_CLI" --erase --id 100 --port "$SERIAL_PORT"
        ;;

    download)
        # Download template from buffer to memory
        exec "$CARBIO_CLI" --down --buffer 1 --file /tmp/carbio_downloaded.bin --port "$SERIAL_PORT"
        ;;

    verify)
        # Verify live finger against template at position 100 (1:1 matching)
        # NOTE: Requires setup script to have been run first (carbio_setup.sh)
        # Use extended timeout for benchmarking (10 seconds) and retry up to 3 times on failure
        RETRY_COUNT=0
        MAX_RETRIES=3
        while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            if "$CARBIO_CLI" --verify --id 100 --port "$SERIAL_PORT" --timeout 10000; then
                exit 0
            fi
            RETRY_COUNT=$((RETRY_COUNT + 1))
            if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
                echo "Retry $RETRY_COUNT/$MAX_RETRIES..." >&2
                sleep 1
            fi
        done
        exit 1
        ;;

    identify)
        # Identify live finger against all stored templates (1:N matching)
        # NOTE: Requires setup script to have been run first (carbio_setup.sh)
        # Use extended timeout for benchmarking (10 seconds) and retry up to 3 times on failure
        RETRY_COUNT=0
        MAX_RETRIES=3
        while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            if "$CARBIO_CLI" --identify --port "$SERIAL_PORT" --timeout 10000; then
                exit 0
            fi
            RETRY_COUNT=$((RETRY_COUNT + 1))
            if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
                echo "Retry $RETRY_COUNT/$MAX_RETRIES..." >&2
                sleep 1
            fi
        done
        exit 1
        ;;

    *)
        echo "Usage: $0 {store|load|delete|download|verify|identify}" >&2
        exit 1
        ;;
esac
