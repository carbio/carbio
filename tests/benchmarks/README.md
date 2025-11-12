# Fingerprint Benchmark Suite

This directory contains a comprehensive benchmark suite for comparing the Carbio fingerprint library against other popular libraries.

## Structure

```
tests/benchmarks/
├── capture_fingerprint.sh     # Captures a reusable fingerprint template
├── run_all_benchmarks.sh      # Main benchmark orchestrator
├── generate_report.py         # Generates comparison reports
├── setup_libraries.sh         # Downloads and sets up external libraries
├── wrappers/                  # Wrapper scripts for each library
│   ├── carbio_wrapper.sh
│   ├── adafruit_wrapper.sh
│   ├── libfprint_wrapper.sh
│   └── pyfingerprint_wrapper.sh
├── arduino_port/              # Arduino library port for benchmarking
│   ├── Makefile
│   └── *.cpp, *.h
├── fingerprint_data/          # EXCLUDED FROM GIT (biometric data)
├── results/                   # EXCLUDED FROM GIT (generated results)
└── external/                  # EXCLUDED FROM GIT (cloned dependencies)
```

## Security Notice

**NEVER commit files from `fingerprint_data/` directory!**

These files contain real biometric data (fingerprint templates) which:
- Cannot be changed like a password
- Are permanently identifying
- Must remain private

The `.gitignore` file is configured to exclude all biometric data.

## Setup

1. Install dependencies:
   ```bash
   ./setup_libraries.sh
   ```

2. Build the Arduino port:
   ```bash
   cd arduino_port
   make
   ```

## Usage

1. Capture a test fingerprint (one time):
   ```bash
   ./capture_fingerprint.sh
   ```

2. Run benchmarks:
   ```bash
   ./run_all_benchmarks.sh
   ```

3. View results in `results/` directory (gitignored by default)

## What Gets Committed

**DO commit these:**
- Shell scripts (*.sh)
- Python scripts (*.py)
- Source code (*.cpp, *.h)
- Makefiles
- Documentation (README.md)

**NEVER commit these:**
- Biometric data (`fingerprint_data/`)
- Compiled binaries (`*.o`, executables)
- External dependencies (`external/`)
- Generated results (`results/`)

## Benchmark Results

Results are generated locally and excluded from git by default. To share results:
- Copy sanitized reports manually
- Use CI/CD for automated benchmarking
- Consider committing baseline results with prefix `baseline_*` (update `.gitignore` accordingly)
