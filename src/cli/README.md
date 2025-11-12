# carbiok - CLI Tool for carbio Fingerprint Library

A comprehensive command-line interface for the carbio fingerprint sensor library.

## Build

```bash
cd /path/to/carbio
cmake --workflow --preset gcc-release  # or your preferred preset
```

The executable will be at: `build/gcc-release/bin/carbiok`

## Overview

`carbiok` provides complete access to all fingerprint sensor operations through command-line flags. It uses gflags for robust parameter parsing and supports all carbio API operations.

## Command Categories

### Low-Level Operations

| Command | Description | Required Params | Optional Params |
|---------|-------------|----------------|-----------------|
| `--capture` | Capture fingerprint image | - | - |
| `--extract` | Extract features to buffer | - | `--buffer` |
| `--merge` | Merge buffers 1+2 into template | - | - |
| `--store` | Store buffer to flash | `--id` | `--buffer` |
| `--load` | Load flash to buffer | `--id` | `--buffer` |
| `--down` | Download buffer to file | - | `--buffer`, `--file` |
| `--up` | Upload file to buffer | `--file` | `--buffer` |
| `--erase` | Erase template(s) from flash | `--id` | `--count` |
| `--erase_all` | Clear entire database | - | - |
| `--match` | Match buffers 1 and 2 | - | - |
| `--search` | Search database (1:N) | - | `--buffer`, `--page`, `--count` |
| `--fast_search` | Fast search database | - | `--buffer`, `--page`, `--count` |
| `--get_count` | Get template count | - | - |

### Template File Operations

| Command | Description | Required Params | Optional Params |
|---------|-------------|----------------|-----------------|
| `--export` | Export flash → file | `--id`, `--file` | `--buffer` |
| `--import` | Import file → flash | `--id`, `--file` | `--buffer` |

### High-Level Workflows

| Command | Description | Required Params | Optional Params |
|---------|-------------|----------------|-----------------|
| `--enroll` | Enroll new fingerprint | `--id` | `--samples` |
| `--verify` | Verify against ID | `--id` | - |
| `--identify` | Identify in database | - | - |

### Utility Commands

| Command | Description | Required Params |
|---------|-------------|----------------|
| `--led 0` | Turn LED off | - |
| `--led 1` | Turn LED on | - |

## Common Parameters

- `--port <path>` - Serial port (default: `/dev/ttyAMA0`)
- `--id <num>` - Template ID/position (default: 0)
- `--buffer <1|2>` - Buffer ID (default: 1)
- `--count <num>` - Count parameter (default: 0 = all)
- `--page <num>` - Page ID for search start (default: 0)
- `--samples <num>` - Sample count for enrollment (default: 12)
- `--file <path>` - File path for I/O operations

## Usage Examples

### Basic Operations

```bash
# Capture image
carbiok --capture

# Extract features to buffer 1
carbiok --extract --buffer 1

# Merge buffers into template
carbiok --merge

# Store buffer 1 to position 5
carbiok --store --id 5 --buffer 1

# Load from position 5 to buffer 2
carbiok --load --id 5 --buffer 2
```

### File Operations

```bash
# Download buffer 1 to file
carbiok --down --buffer 1 --file template.bin

# Upload file to buffer 1
carbiok --up --file template.bin --buffer 1

# Export template from flash position 10 to file
carbiok --export --id 10 --file fingerprint_10.bin

# Import template from file to flash position 20
carbiok --import --id 20 --file fingerprint_10.bin
```

### High-Level Workflows

```bash
# Enroll fingerprint at position 5
carbiok --enroll --id 5 --samples 12

# Verify against position 5
carbiok --verify --id 5

# Identify fingerprint (search entire database)
carbiok --identify
```

### Database Operations

```bash
# Search from buffer 1, starting at page 0, search 200 positions
carbiok --search --buffer 1 --page 0 --count 200

# Fast search
carbiok --fast_search --buffer 1

# Get template count
carbiok --get_count

# Delete template at position 5
carbiok --erase --id 5

# Delete 10 templates starting from position 5
carbiok --erase --id 5 --count 10

# Clear entire database
carbiok --erase_all
```

### Utility

```bash
# Turn LED on
carbiok --led 1

# Turn LED off
carbiok --led 0
```

### Different Serial Port

```bash
# Use USB serial adapter
carbiok --port /dev/ttyUSB0 --capture

# Use custom port
carbiok --port /dev/serial0 --enroll --id 1
```

## Workflow Examples

### Complete Enrollment Workflow (Manual)

```bash
# Step 1: Capture first image
carbiok --capture

# Step 2: Extract features to buffer 1
carbiok --extract --buffer 1

# Step 3: Capture second image
carbiok --capture

# Step 4: Extract features to buffer 2
carbiok --extract --buffer 2

# Step 5: Merge into template
carbiok --merge

# Step 6: Store to position 5
carbiok --store --id 5 --buffer 1
```

### Complete Enrollment Workflow (Automated)

```bash
# One command does it all!
carbiok --enroll --id 5 --samples 12
```

### Backup/Restore Templates

```bash
# Backup template 5
carbiok --export --id 5 --file backup_template_5.bin

# Restore to position 10
carbiok --import --id 10 --file backup_template_5.bin
```

### Verification Workflow

```bash
# Option 1: High-level (automated)
carbiok --verify --id 5

# Option 2: Manual steps
carbiok --capture
carbiok --extract --buffer 1
carbiok --load --id 5 --buffer 2
carbiok --match
```

### Identification Workflow

```bash
# Option 1: High-level (automated)
carbiok --identify

# Option 2: Manual steps
carbiok --capture
carbiok --extract --buffer 1
carbiok --fast_search --buffer 1
```

## Exit Codes

- `0` - Success
- `1` - Failure (sensor error, command failed, invalid parameters)

## Error Handling

All commands provide clear error messages:

```bash
$ carbiok --export --id 5
Error: --file required for export

$ carbiok --capture --enroll
Error: Multiple commands specified, only one allowed
```

## Integration with Benchmarking

This CLI is designed to be easily integrated with benchmarking tools like hyperfine:

```bash
# Benchmark enrollment
hyperfine --warmup 1 --runs 10 'carbiok --enroll --id 100'

# Benchmark verification
hyperfine --runs 10 'carbiok --verify --id 100'

# Benchmark identification
hyperfine --runs 10 'carbiok --identify'
```

## Notes

- Only one command can be specified per invocation
- Serial port auto-detects baud rate (tries common rates)
- All parameters are properly type-checked and validated
- Progress callbacks show real-time status for high-level commands
- File I/O uses binary mode for exact template preservation
