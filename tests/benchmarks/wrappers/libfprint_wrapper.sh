#!/usr/bin/env bash
#
# libfprint Library Wrapper for Hyperfine Benchmarking
#

set -euo pipefail

OPERATION="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FP_DATA="${SCRIPT_DIR}/../fingerprint_data/captured_template.bin"
LIBFPRINT_DIR="${SCRIPT_DIR}/../external/libfprint"

# Note: libfprint uses D-Bus and works with USB fingerprint readers
# It may not directly support UART sensors like R30x
# This wrapper provides a stub for completeness

echo "Warning: libfprint typically requires USB fingerprint readers" >&2
echo "R30x UART sensors may not be supported directly" >&2

case "$OPERATION" in
    enroll|verify|identify|delete)
        echo "libfprint operation: $OPERATION (not implemented for UART sensors)" >&2
        echo "Skipping..." >&2
        exit 0
        ;;
    *)
        echo "Usage: $0 {enroll|verify|identify|delete}" >&2
        exit 1
        ;;
esac
