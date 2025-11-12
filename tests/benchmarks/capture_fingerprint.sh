#!/usr/bin/env bash
#
# Shared Fingerprint Capture Utility
# Captures ONE fingerprint and saves it in a reusable format
# This fingerprint is then reused across all library benchmarks
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_FILE="${1:-${SCRIPT_DIR}/fingerprint_data/captured_template.bin}"
CARBIO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $*"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $*"; }

echo ""
echo "========================================================================"
echo "  Fingerprint Capture Utility"
echo "========================================================================"
echo ""
echo "This will capture ONE fingerprint from your finger."
echo "This fingerprint will be reused across all library benchmarks."
echo ""
echo "Please choose a clean, dry finger (recommend: right index finger)."
echo ""
read -p "Press Enter to continue..."
echo ""

# Detect serial port
SERIAL_PORT=""
if [[ -e /dev/ttyAMA0 ]]; then
    SERIAL_PORT="/dev/ttyAMA0"
elif [[ -e /dev/ttyUSB0 ]]; then
    SERIAL_PORT="/dev/ttyUSB0"
else
    log_warning "No serial port detected at /dev/ttyAMA0 or /dev/ttyUSB0"
    read -p "Enter serial port path: " SERIAL_PORT
fi

log_info "Using serial port: $SERIAL_PORT"

# Find carbio CLI tool
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
    log_warning "carbio CLI not found, trying Python-based capture..."

    # Create a simple Python-based capture script
    cat > /tmp/capture_fp.py <<'PYTHON_SCRIPT'
#!/usr/bin/env python3
import sys
import time

try:
    import serial
except ImportError:
    print("Error: pyserial not installed. Run: pip3 install pyserial")
    sys.exit(1)

def capture_fingerprint(port, output_file):
    """Capture fingerprint using R30x sensor protocol"""
    print(f"Opening serial port: {port}")
    ser = serial.Serial(port, 57600, timeout=2)

    # Clear buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.2)

    print("\nPlace your finger on the sensor...")

    # Image capture command (0x01 = GenImg)
    capture_cmd = bytes([
        0xEF, 0x01,                 # Header
        0xFF, 0xFF, 0xFF, 0xFF,     # Address
        0x01,                        # Package ID
        0x00, 0x03,                 # Length
        0x01,                        # GenImg instruction
        0x00, 0x05                  # Checksum
    ])

    ser.write(capture_cmd)
    time.sleep(1.0)

    response = ser.read(12)
    if len(response) < 12 or response[9] != 0x00:
        print("Error: Failed to capture image")
        ser.close()
        return False

    print("Image captured successfully")

    # Convert to CharBuffer1 (0x02 = Img2Tz)
    print("Extracting features...")
    img2tz_cmd = bytes([
        0xEF, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x01,
        0x00, 0x04,
        0x02, 0x01,                 # Img2Tz, BufferID=1
        0x00, 0x08
    ])

    ser.write(img2tz_cmd)
    time.sleep(0.5)

    response = ser.read(12)
    if len(response) < 12 or response[9] != 0x00:
        print("Error: Failed to extract features")
        ser.close()
        return False

    print("Features extracted successfully")

    # Upload template (0x08 = UpChar)
    print("Reading template...")
    upload_cmd = bytes([
        0xEF, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x01,
        0x00, 0x04,
        0x08, 0x01,                 # UpChar, BufferID=1
        0x00, 0x0E
    ])

    ser.write(upload_cmd)
    time.sleep(0.1)

    # Read template data
    template_data = bytearray()
    while True:
        packet = ser.read(128)
        if len(packet) == 0:
            break
        template_data.extend(packet)
        # Check for end packet (PID = 0x08)
        if len(packet) >= 9 and packet[6] == 0x08:
            break

    ser.close()

    if len(template_data) < 100:
        print(f"Error: Template too small ({len(template_data)} bytes)")
        return False

    # Save template
    with open(output_file, 'wb') as f:
        f.write(template_data)

    print(f"\nFingerprint saved: {output_file}")
    print(f"Template size: {len(template_data)} bytes")

    return True

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: capture_fp.py <serial_port> <output_file>")
        sys.exit(1)

    port = sys.argv[1]
    output = sys.argv[2]

    success = capture_fingerprint(port, output)
    sys.exit(0 if success else 1)
PYTHON_SCRIPT

    chmod +x /tmp/capture_fp.py

    # Run Python capture
    if python3 /tmp/capture_fp.py "$SERIAL_PORT" "$OUTPUT_FILE"; then
        log_success "Fingerprint captured successfully"
        rm -f /tmp/capture_fp.py
        exit 0
    else
        log_warning "Python capture failed"
        rm -f /tmp/capture_fp.py
        exit 1
    fi
else
    log_info "Using carbio CLI: $CARBIO_CLI"

    # Use carbio to capture and export fingerprint
    mkdir -p "$(dirname "$OUTPUT_FILE")"

    # Enroll fingerprint and save template
    log_info "Starting enrollment..."
    echo ""

    # Use ID 149 (max capacity is 150, range 0-149)
    TEMP_ID=149

    "$CARBIO_CLI" --enroll --id "$TEMP_ID" --port "$SERIAL_PORT" || {
        log_warning "Enrollment failed"
        exit 1
    }

    # Export template to file
    log_info "Exporting template..."
    "$CARBIO_CLI" --export --id "$TEMP_ID" --file "$OUTPUT_FILE" --port "$SERIAL_PORT" || {
        log_warning "Export failed"
        exit 1
    }

    log_success "Fingerprint captured and saved: $OUTPUT_FILE"
fi

echo ""
log_info "Template saved to: $OUTPUT_FILE"
log_info "This fingerprint will be reused for all benchmarks"
echo ""
