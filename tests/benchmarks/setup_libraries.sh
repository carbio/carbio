#!/usr/bin/env bash
#
# Setup External Fingerprint Libraries
# Clones, builds, and prepares all external libraries for benchmarking
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTERNAL_DIR="${SCRIPT_DIR}/external"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $*"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

echo ""
echo "========================================================================"
echo "  External Library Setup"
echo "========================================================================"
echo ""

mkdir -p "$EXTERNAL_DIR"
cd "$EXTERNAL_DIR"

# ============================================================================
# libfprint
# ============================================================================
setup_libfprint() {
    log_info "Setting up libfprint..."

    if [[ -d "libfprint" ]]; then
        log_info "libfprint already cloned, skipping..."
        return 0
    fi

    git clone https://gitlab.freedesktop.org/libfprint/libfprint.git
    cd libfprint

    # Install dependencies
    log_info "Installing libfprint dependencies..."
    sudo apt-get install -y \
        meson \
        libglib2.0-dev \
        libgusb-dev \
        libnss3-dev \
        gobject-introspection \
        libgirepository1.0-dev \
        gtk-doc-tools

    # Build
    log_info "Building libfprint..."
    meson setup builddir
    meson compile -C builddir

    log_success "libfprint setup complete"
    cd "$EXTERNAL_DIR"
}

# ============================================================================
# Adafruit Fingerprint Sensor Library
# ============================================================================
setup_adafruit() {
    log_info "Setting up Adafruit Fingerprint Library..."

    if [[ -d "Adafruit-Fingerprint-Sensor-Library" ]]; then
        log_info "Adafruit library already cloned, skipping..."
        return 0
    fi

    git clone https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library.git

    # Note: This is an Arduino library, we'll use the existing port in carbio
    # Or create a minimal C++ wrapper

    log_success "Adafruit library setup complete"
    cd "$EXTERNAL_DIR"
}

# ============================================================================
# pyfingerprint
# ============================================================================
setup_pyfingerprint() {
    log_info "Setting up pyfingerprint..."

    if [[ -d "pyfingerprint" ]]; then
        log_info "pyfingerprint already cloned, skipping..."
        return 0
    fi

    git clone https://github.com/bastianraschke/pyfingerprint.git

    # Install Python dependencies
    log_info "Installing pyfingerprint dependencies..."
    pip3 install --user pyserial Pillow

    log_success "pyfingerprint setup complete"
    cd "$EXTERNAL_DIR"
}

# ============================================================================
# Main execution
# ============================================================================

main() {
    log_info "Starting library setup in: $EXTERNAL_DIR"
    echo ""

    # Update package lists
    log_info "Updating apt package lists..."
    sudo apt-get update -qq

    # Setup each library (run in parallel where safe, or sequentially)
    setup_libfprint || log_warning "libfprint setup had issues"
    echo ""

    setup_adafruit || log_warning "Adafruit setup had issues"
    echo ""

    setup_pyfingerprint || log_warning "pyfingerprint setup had issues"
    echo ""

    echo "========================================================================"
    echo "  Library Setup Complete"
    echo "========================================================================"
    echo ""
    log_info "Installed libraries:"
    ls -1 "$EXTERNAL_DIR"
    echo ""
    log_success "All libraries ready for benchmarking"
}

main "$@"
