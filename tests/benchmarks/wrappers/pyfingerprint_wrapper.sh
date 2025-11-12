#!/usr/bin/env bash
#
# pyfingerprint Library Wrapper for Hyperfine Benchmarking
# Template Management Operations Only (No Live Finger Required)
#

set -euo pipefail

OPERATION="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FP_DATA="${SCRIPT_DIR}/../fingerprint_data/captured_template.bin"
PYFINGERPRINT_DIR="${SCRIPT_DIR}/../external/pyfingerprint"

# Create wrapper Python script
cat > /tmp/pyfingerprint_bench.py <<'PYTHON_SCRIPT'
#!/usr/bin/env python3
import sys
import os

# Add pyfingerprint to path
pyf_dir = os.environ.get('PYFINGERPRINT_DIR', '.')
sys.path.insert(0, os.path.join(pyf_dir, 'src', 'files'))

from pyfingerprint.pyfingerprint import PyFingerprint
from pyfingerprint.pyfingerprint import FINGERPRINT_CHARBUFFER1, FINGERPRINT_CHARBUFFER2

def get_sensor(port='/dev/ttyAMA0'):
    """Initialize fingerprint sensor"""
    import time
    if not os.path.exists(port):
        port = '/dev/ttyUSB0'

    # Add retry logic for sensor initialization (handles serial port sync issues)
    max_retries = 3
    for attempt in range(max_retries):
        try:
            f = PyFingerprint(port, 57600, 0xFFFFFFFF, 0x00000000)
            if not f.verifyPassword():
                raise ValueError('Sensor password incorrect')
            return f
        except Exception as e:
            if attempt < max_retries - 1:
                # Wait a bit and retry (allows serial port to reset)
                time.sleep(0.1)
                continue
            else:
                raise e

def store_template(fp_file):
    """Upload template from file and store to sensor position 100"""
    f = get_sensor()

    with open(fp_file, 'rb') as file:
        template_data = file.read()

    # Upload characteristics to buffer 1
    f.uploadCharacteristics(FINGERPRINT_CHARBUFFER1, template_data)

    # Store from buffer 1 to position 100
    position = f.storeTemplate(100, FINGERPRINT_CHARBUFFER1)
    print(f"Stored template at position {position}")

def load_template():
    """Load template from position 100 to buffer 1"""
    f = get_sensor()

    # Load template from position 100 to buffer 1
    f.loadTemplate(100, FINGERPRINT_CHARBUFFER1)
    print("Loaded template from position 100")

def delete_template():
    """Delete template at position 100"""
    f = get_sensor()

    # Delete 1 template starting at position 100
    f.deleteTemplate(100, 1)
    print("Deleted template at position 100")

def download_template():
    """Download template from buffer 1 to memory"""
    f = get_sensor()

    # Download characteristics from buffer 1
    characteristics = f.downloadCharacteristics(FINGERPRINT_CHARBUFFER1)

    # Save to temp file (for benchmarking purposes)
    with open('/tmp/pyfingerprint_downloaded.bin', 'wb') as file:
        file.write(bytearray(characteristics))

    print(f"Downloaded template ({len(characteristics)} bytes)")

def verify_fingerprint():
    """Verify live finger against template at position 100 (1:1 matching)"""
    f = get_sensor()

    print("Place finger on sensor...")

    # Wait for finger
    while not f.readImage():
        pass

    # Convert image to characteristics in buffer 1
    f.convertImage(FINGERPRINT_CHARBUFFER1)

    # Load stored template from position 100 to buffer 2
    f.loadTemplate(100, FINGERPRINT_CHARBUFFER2)

    # Compare buffer 1 (live) with buffer 2 (stored)
    score = f.compareCharacteristics()

    if score > 0:
        print(f"Verification SUCCESS (score: {score})")
    else:
        print("Verification FAILED")

def identify_fingerprint():
    """Identify live finger against all stored templates (1:N matching)"""
    f = get_sensor()

    print("Place finger on sensor...")

    # Wait for finger
    while not f.readImage():
        pass

    # Convert image to characteristics in buffer 1
    f.convertImage(FINGERPRINT_CHARBUFFER1)

    # Search template database
    result = f.searchTemplate()
    position = result[0]
    score = result[1]

    if position >= 0:
        print(f"Identification SUCCESS (position: {position}, score: {score})")
    else:
        print("Identification FAILED (no match)")

if __name__ == '__main__':
    operation = sys.argv[1] if len(sys.argv) > 1 else ''
    fp_file = sys.argv[2] if len(sys.argv) > 2 else ''

    try:
        if operation == 'store':
            store_template(fp_file)
        elif operation == 'load':
            load_template()
        elif operation == 'delete':
            delete_template()
        elif operation == 'download':
            download_template()
        elif operation == 'verify':
            verify_fingerprint()
        elif operation == 'identify':
            identify_fingerprint()
        else:
            print(f"Unknown operation: {operation}")
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
PYTHON_SCRIPT

chmod +x /tmp/pyfingerprint_bench.py

# Run operation
export PYFINGERPRINT_DIR="$PYFINGERPRINT_DIR"

case "$OPERATION" in
    store)
        exec python3 /tmp/pyfingerprint_bench.py store "$FP_DATA"
        ;;
    load|delete|download|verify|identify)
        exec python3 /tmp/pyfingerprint_bench.py "$OPERATION"
        ;;
    *)
        echo "Usage: $0 {store|load|delete|download|verify|identify}" >&2
        exit 1
        ;;
esac
