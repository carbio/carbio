#!/usr/bin/env bash
#
# Multi-Library Fingerprint Benchmark Suite
# Uses hyperfine for statistical rigor and reproducibility
#
# Features:
# - Captures ONE fingerprint and reuses it across all tests
# - Hyperfine provides warmup, multiple runs, outlier detection
# - Generates JSON output and formatted comparison report
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="${SCRIPT_DIR}/results"
FP_DATA_DIR="${SCRIPT_DIR}/fingerprint_data"
WRAPPERS_DIR="${SCRIPT_DIR}/wrappers"
EXTERNAL_DIR="${SCRIPT_DIR}/external"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
BENCHMARK_JSON="${RESULTS_DIR}/benchmark_${TIMESTAMP}.json"
REPORT_MD="${RESULTS_DIR}/comparison_report_${TIMESTAMP}.md"

# Benchmark configuration - Hybrid Approach (Option B)
# Automated operations (template management): 100 runs for reliable P99 analysis
# Live operations (verify/identify): 15 runs (requires manual finger placement)
HYPERFINE_RUNS_AUTOMATED=100
HYPERFINE_RUNS_LIVE=15
HYPERFINE_WARMUP=5
HYPERFINE_EXPORT_JSON="${BENCHMARK_JSON}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $*"
}

print_banner() {
    echo ""
    echo "========================================================================"
    echo "  Multi-Library Fingerprint Performance Benchmark Suite"
    echo "  Powered by hyperfine - Raspberry Pi 5 Edition"
    echo "========================================================================"
    echo ""
}

check_prerequisites() {
    log_info "Checking prerequisites..."

    local missing=0

    # Check hyperfine
    if ! command -v hyperfine &> /dev/null; then
        log_error "hyperfine not found. Install with: sudo apt install hyperfine"
        missing=1
    else
        log_success "hyperfine: $(hyperfine --version)"
    fi

    # Check build tools
    for tool in gcc g++ cmake make python3; do
        if ! command -v "$tool" &> /dev/null; then
            log_error "$tool not found. Install with: sudo apt install $tool"
            missing=1
        fi
    done

    # Check sensor
    if [[ ! -e /dev/ttyAMA0 ]] && [[ ! -e /dev/ttyUSB0 ]]; then
        log_warning "Sensor not detected at /dev/ttyAMA0 or /dev/ttyUSB0"
        log_warning "Make sure your fingerprint sensor is connected"
    else
        log_success "Sensor detected"
    fi

    if [[ $missing -eq 1 ]]; then
        log_error "Please install missing prerequisites"
        exit 1
    fi

    log_success "All prerequisites met"
}

setup_directories() {
    log_info "Setting up directories..."
    mkdir -p "$RESULTS_DIR"
    mkdir -p "$FP_DATA_DIR"
    mkdir -p "$WRAPPERS_DIR"
    mkdir -p "$EXTERNAL_DIR"
    log_success "Directories ready"
}

setup_libraries() {
    log_info "Setting up external libraries..."

    if [[ -f "${SCRIPT_DIR}/setup_libraries.sh" ]]; then
        bash "${SCRIPT_DIR}/setup_libraries.sh"
    else
        log_warning "setup_libraries.sh not found, skipping library setup"
    fi

    log_success "Library setup complete"
}

capture_or_reuse_fingerprint() {
    local fp_file="${FP_DATA_DIR}/captured_template.bin"

    if [[ -f "$fp_file" ]]; then
        log_info "Found existing fingerprint data: $fp_file"
        echo ""
        read -p "Reuse existing fingerprint? [Y/n]: " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
            log_success "Reusing existing fingerprint"
            return 0
        fi
    fi

    log_info "Capturing new fingerprint..."

    if [[ -f "${SCRIPT_DIR}/capture_fingerprint.sh" ]]; then
        bash "${SCRIPT_DIR}/capture_fingerprint.sh" "$fp_file"
    else
        log_error "capture_fingerprint.sh not found"
        exit 1
    fi

    if [[ ! -f "$fp_file" ]]; then
        log_error "Fingerprint capture failed"
        exit 1
    fi

    log_success "Fingerprint captured and saved"
}

run_benchmarks() {
    log_info "Starting hybrid benchmarks (Option B)..."
    echo ""
    echo "Configuration:"
    echo "  Automated operations (store/load/delete/download): $HYPERFINE_RUNS_AUTOMATED runs"
    echo "  Live operations (verify/identify): $HYPERFINE_RUNS_LIVE runs (requires finger placement)"
    echo "  Warmup runs: $HYPERFINE_WARMUP"
    echo ""

    local automated_json="${RESULTS_DIR}/automated_${TIMESTAMP}.json"
    local live_json="${RESULTS_DIR}/live_${TIMESTAMP}.json"
    local automated_md="${RESULTS_DIR}/automated_${TIMESTAMP}.md"
    local live_md="${RESULTS_DIR}/live_${TIMESTAMP}.md"

    # Phase 1: Automated template management operations (100 runs)
    log_info "PHASE 1: Automated Template Management Operations"
    log_info "This phase runs fully automated - no user interaction needed"
    echo ""

    local automated_benchmarks=()

    # carbio automated operations
    if [[ -f "${WRAPPERS_DIR}/carbio_wrapper.sh" ]]; then
        automated_benchmarks+=("--command-name" "carbio_store")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh store")

        automated_benchmarks+=("--command-name" "carbio_load")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh load")

        automated_benchmarks+=("--command-name" "carbio_delete")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh delete")

        automated_benchmarks+=("--command-name" "carbio_download")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh download")
    fi

    # Adafruit automated operations
    if [[ -f "${WRAPPERS_DIR}/adafruit_wrapper.sh" ]]; then
        automated_benchmarks+=("--command-name" "adafruit_store")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh store")

        automated_benchmarks+=("--command-name" "adafruit_load")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh load")

        automated_benchmarks+=("--command-name" "adafruit_delete")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh delete")

        automated_benchmarks+=("--command-name" "adafruit_download")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh download")
    fi

    # pyfingerprint automated operations
    if [[ -f "${WRAPPERS_DIR}/pyfingerprint_wrapper.sh" ]]; then
        automated_benchmarks+=("--command-name" "pyfingerprint_store")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh store")

        automated_benchmarks+=("--command-name" "pyfingerprint_load")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh load")

        automated_benchmarks+=("--command-name" "pyfingerprint_delete")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh delete")

        automated_benchmarks+=("--command-name" "pyfingerprint_download")
        automated_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh download")
    fi

    if [[ ${#automated_benchmarks[@]} -gt 0 ]]; then
        log_info "Running ${#automated_benchmarks[@]} automated benchmarks with $HYPERFINE_RUNS_AUTOMATED runs each..."
        hyperfine \
            --runs "$HYPERFINE_RUNS_AUTOMATED" \
            --warmup "$HYPERFINE_WARMUP" \
            --export-json "$automated_json" \
            --export-markdown "$automated_md" \
            --style full \
            "${automated_benchmarks[@]}"
        log_success "Phase 1 complete"
    else
        log_warning "No automated benchmarks found"
    fi

    echo ""
    echo "========================================================================"
    log_info "PHASE 2: Live Authentication Operations"
    log_warning "ATTENTION: You will need to place your finger on the sensor $HYPERFINE_RUNS_LIVE times per library"
    log_warning "Total finger placements required: approximately $(( HYPERFINE_RUNS_LIVE * 2 * 4 )) times"
    echo "========================================================================"
    echo ""
    read -p "Press ENTER when ready to start live operations..." -r
    echo ""

    # Run setup scripts for each library to ensure templates are in place
    log_info "Running pre-benchmark setup..."
    if [[ -f "${WRAPPERS_DIR}/carbio_setup.sh" ]]; then
        bash "${WRAPPERS_DIR}/carbio_setup.sh" || log_warning "carbio setup failed"
    fi
    if [[ -f "${WRAPPERS_DIR}/adafruit_setup.sh" ]]; then
        bash "${WRAPPERS_DIR}/adafruit_setup.sh" || log_warning "adafruit setup failed"
    fi
    # Add setup scripts for other libraries here if needed
    log_success "Setup complete"
    echo ""

    local live_benchmarks=()

    # carbio live operations
    if [[ -f "${WRAPPERS_DIR}/carbio_wrapper.sh" ]]; then
        live_benchmarks+=("--command-name" "carbio_verify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh verify")

        live_benchmarks+=("--command-name" "carbio_identify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/carbio_wrapper.sh identify")
    fi

    # Adafruit live operations
    if [[ -f "${WRAPPERS_DIR}/adafruit_wrapper.sh" ]]; then
        live_benchmarks+=("--command-name" "adafruit_verify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh verify")

        live_benchmarks+=("--command-name" "adafruit_identify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/adafruit_wrapper.sh identify")
    fi

    # pyfingerprint live operations
    if [[ -f "${WRAPPERS_DIR}/pyfingerprint_wrapper.sh" ]]; then
        live_benchmarks+=("--command-name" "pyfingerprint_verify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh verify")

        live_benchmarks+=("--command-name" "pyfingerprint_identify")
        live_benchmarks+=("bash ${WRAPPERS_DIR}/pyfingerprint_wrapper.sh identify")
    fi

    if [[ ${#live_benchmarks[@]} -gt 0 ]]; then
        log_info "Running ${#live_benchmarks[@]} live benchmarks with $HYPERFINE_RUNS_LIVE runs each..."
        log_warning "Please place your finger when prompted!"
        hyperfine \
            --runs "$HYPERFINE_RUNS_LIVE" \
            --warmup 2 \
            --ignore-failure \
            --export-json "$live_json" \
            --export-markdown "$live_md" \
            --show-output \
            "${live_benchmarks[@]}"
        log_success "Phase 2 complete"
    else
        log_warning "No live benchmarks found"
    fi

    # Merge JSON results
    if [[ -f "$automated_json" ]] && [[ -f "$live_json" ]]; then
        log_info "Merging results..."
        python3 -c "
import json
with open('$automated_json') as f1, open('$live_json') as f2:
    auto = json.load(f1)
    live = json.load(f2)
    auto['results'].extend(live['results'])
    with open('$HYPERFINE_EXPORT_JSON', 'w') as out:
        json.dump(auto, out, indent=2)
"
        log_success "Results merged into $HYPERFINE_EXPORT_JSON"
    elif [[ -f "$automated_json" ]]; then
        cp "$automated_json" "$HYPERFINE_EXPORT_JSON"
    elif [[ -f "$live_json" ]]; then
        cp "$live_json" "$HYPERFINE_EXPORT_JSON"
    else
        log_error "No benchmark results found"
        exit 1
    fi

    log_success "All benchmarks complete"
}

generate_report() {
    log_info "Generating comprehensive comparison report..."

    if [[ -f "${SCRIPT_DIR}/generate_report.py" ]]; then
        python3 "${SCRIPT_DIR}/generate_report.py" "$BENCHMARK_JSON" "$REPORT_MD"
    else
        log_warning "generate_report.py not found, using hyperfine markdown output only"
        cp "${RESULTS_DIR}/quick_comparison_${TIMESTAMP}.md" "$REPORT_MD"
    fi

    log_success "Report generated: $REPORT_MD"
}

print_results() {
    echo ""
    echo "========================================================================"
    echo "  BENCHMARK COMPLETE"
    echo "========================================================================"
    echo ""
    echo "Results:"
    echo "  JSON:   $BENCHMARK_JSON"
    echo "  Report: $REPORT_MD"
    echo ""
    echo "Quick view:"
    cat "${RESULTS_DIR}/quick_comparison_${TIMESTAMP}.md"
    echo ""
    log_success "All benchmarks completed successfully!"
}

main() {
    print_banner
    check_prerequisites
    setup_directories
    setup_libraries
    capture_or_reuse_fingerprint
    run_benchmarks
    generate_report
    print_results
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
