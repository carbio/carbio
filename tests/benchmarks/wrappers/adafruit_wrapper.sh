#!/usr/bin/env bash
#
# Adafruit Fingerprint Library Wrapper for Hyperfine Benchmarking
# Provides consistent CLI interface for benchmarking operations
#

set -euo pipefail

OPERATION="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FP_DATA="${SCRIPT_DIR}/../fingerprint_data/captured_template.bin"
ARDUINO_PORT_DIR="${SCRIPT_DIR}/../arduino_port"
TEMPLATE_OPS="${ARDUINO_PORT_DIR}/template_ops"

# Ensure template_ops is built
if [[ ! -x "$TEMPLATE_OPS" ]]; then
    echo "Building template_ops..." >&2
    (cd "$ARDUINO_PORT_DIR" && make template_ops) || {
        echo "Error: Failed to build template_ops" >&2
        exit 1
    }
fi

case "$OPERATION" in
    store)
        exec "$TEMPLATE_OPS" store "$FP_DATA"
        ;;

    load)
        exec "$TEMPLATE_OPS" load
        ;;

    delete)
        exec "$TEMPLATE_OPS" delete
        ;;

    download)
        exec "$TEMPLATE_OPS" download
        ;;

    verify)
        # Verify live finger against template at position 100 (1:1 matching)
        # NOTE: Requires setup script to have been run first (adafruit_setup.sh)
        # Use retry logic to handle transient failures
        RETRY_COUNT=0
        MAX_RETRIES=3
        while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            if "$TEMPLATE_OPS" verify; then
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
        # NOTE: Requires setup script to have been run first (adafruit_setup.sh)
        # Use retry logic to handle transient failures
        RETRY_COUNT=0
        MAX_RETRIES=3
        while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            if "$TEMPLATE_OPS" identify; then
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
