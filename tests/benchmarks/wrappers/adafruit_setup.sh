#!/usr/bin/env bash
#
# Adafruit Benchmark Setup Script
# Ensures templates are in place before benchmarks run
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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

echo "Setting up Adafruit benchmarks..."

# Ensure template at position 100 for verify/identify benchmarks
# Copy from reference position 149 (created during fingerprint capture)
echo "  - Copying reference template to position 100..."
"$TEMPLATE_OPS" load > /dev/null 2>&1 || {
    echo "  - Loading reference template from position 149..."
    # If position 100 doesn't exist, copy from 149
    # This requires that position 149 was set up during capture
    if ! "$TEMPLATE_OPS" store /dev/null 2>&1; then
        echo "Warning: Failed to setup template at position 100" >&2
        echo "Note: Make sure a template exists at position 149" >&2
        exit 1
    fi
}

echo "Setup complete!"
