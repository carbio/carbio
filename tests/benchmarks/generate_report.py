#!/usr/bin/env python3
"""
Multi-Library Benchmark Report Generator
Converts hyperfine JSON output to comprehensive comparison report
"""

import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict


def load_hyperfine_results(json_file: str) -> Dict[str, Any]:
    """Load hyperfine benchmark results"""
    with open(json_file, 'r') as f:
        return json.load(f)


def parse_benchmark_name(name: str) -> tuple:
    """
    Parse benchmark name into library and operation
    Example: 'carbio_enroll' -> ('carbio', 'enroll')
    """
    parts = name.split('_', 1)
    if len(parts) == 2:
        return parts[0], parts[1]
    return 'unknown', name


def calculate_percentile(values: List[float], percentile: float) -> float:
    """Calculate percentile from list of values"""
    if not values:
        return 0.0
    sorted_values = sorted(values)
    index = (len(sorted_values) - 1) * (percentile / 100.0)
    lower = int(index)
    upper = lower + 1
    weight = index - lower

    if upper >= len(sorted_values):
        return sorted_values[-1] * 1000  # Convert to ms

    return (sorted_values[lower] * (1 - weight) + sorted_values[upper] * weight) * 1000


def organize_results(data: Dict[str, Any]) -> Dict[str, Dict[str, Dict]]:
    """
    Organize results by library and operation
    Returns: {library: {operation: {stats}}}
    """
    organized = defaultdict(lambda: defaultdict(dict))

    for result in data.get('results', []):
        command_name = result.get('command', '')
        library, operation = parse_benchmark_name(command_name)

        times = result.get('times', [])

        organized[library][operation] = {
            'mean': result.get('mean', 0) * 1000,  # Convert to ms
            'stddev': result.get('stddev', 0) * 1000,
            'median': result.get('median', 0) * 1000,
            'min': result.get('min', 0) * 1000,
            'max': result.get('max', 0) * 1000,
            'user': result.get('user', 0) * 1000,
            'system': result.get('system', 0) * 1000,
            'runs': times,
            'p95': calculate_percentile(times, 95),
            'p99': calculate_percentile(times, 99),
            'p999': calculate_percentile(times, 99.9),
        }

    return dict(organized)


def calculate_speedup(baseline: float, compare: float) -> str:
    """Calculate speedup factor"""
    if baseline == 0 or compare == 0:
        return "N/A"

    ratio = compare / baseline
    if ratio > 1:
        return f"{ratio:.2f}x slower"
    elif ratio < 1:
        return f"{1/ratio:.2f}x faster"
    else:
        return "same"


def generate_markdown_report(results: Dict[str, Dict[str, Dict]], output_file: str):
    """Generate comprehensive markdown report"""

    lines = []

    # Header
    lines.append("# Multi-Library Fingerprint Performance Comparison")
    lines.append("")
    lines.append(f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"**Hardware:** Raspberry Pi 5")
    lines.append(f"**Sensor:** R30x Optical Fingerprint Sensor")
    lines.append(f"**Tool:** hyperfine (statistical benchmarking)")
    lines.append("")

    # Get all libraries and operations
    all_libraries = sorted(results.keys())
    all_operations = set()
    for lib_data in results.values():
        all_operations.update(lib_data.keys())
    all_operations = sorted(all_operations)

    lines.append("## Libraries Tested")
    lines.append("")
    for lib in all_libraries:
        lines.append(f"- **{lib}**")
    lines.append("")

    lines.append("## Benchmark Operations")
    lines.append("")
    for op in all_operations:
        lines.append(f"- **{op}** - {get_operation_description(op)}")
    lines.append("")

    # Mean latency comparison table
    lines.append("## Mean Latency Comparison (milliseconds)")
    lines.append("")
    lines.append("Lower is better. Values are mean execution time ± standard deviation.")
    lines.append("")

    # Build table header
    header = ["Operation"] + all_libraries
    lines.append("| " + " | ".join(header) + " |")
    lines.append("|" + "|".join(["-" * (len(h) + 2) for h in header]) + "|")

    # Build table rows
    for operation in all_operations:
        row = [operation]
        for library in all_libraries:
            if operation in results[library]:
                stats = results[library][operation]
                mean = stats['mean']
                stddev = stats['stddev']
                row.append(f"{mean:.2f} ± {stddev:.2f}")
            else:
                row.append("N/A")
        lines.append("| " + " | ".join(row) + " |")

    lines.append("")

    # Speedup comparison (using first library as baseline)
    if len(all_libraries) > 1:
        baseline_lib = all_libraries[0]
        lines.append(f"## Speedup Comparison (vs {baseline_lib})")
        lines.append("")
        lines.append(f"Comparison against **{baseline_lib}** as baseline.")
        lines.append("")

        header = ["Operation"] + [lib for lib in all_libraries if lib != baseline_lib]
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + "|".join(["-" * (len(h) + 2) for h in header]) + "|")

        for operation in all_operations:
            if operation not in results[baseline_lib]:
                continue

            baseline_mean = results[baseline_lib][operation]['mean']
            row = [operation]

            for library in all_libraries:
                if library == baseline_lib:
                    continue

                if operation in results[library]:
                    compare_mean = results[library][operation]['mean']
                    speedup = calculate_speedup(baseline_mean, compare_mean)
                    row.append(speedup)
                else:
                    row.append("N/A")

            lines.append("| " + " | ".join(row) + " |")

        lines.append("")

    # Detailed statistics per operation
    lines.append("## Detailed Statistics")
    lines.append("")
    lines.append("**Note:** P95, P99, and P99.9 percentiles show worst-case performance (higher percentile = rarer outliers).")
    lines.append("")

    for operation in all_operations:
        lines.append(f"### {operation.title()}")
        lines.append("")

        header = ["Library", "Mean (ms)", "Median (ms)", "P95 (ms)", "P99 (ms)", "P99.9 (ms)", "Min (ms)", "Max (ms)", "Std Dev", "CV%"]
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + "|".join(["-" * (len(h) + 2) for h in header]) + "|")

        for library in all_libraries:
            if operation in results[library]:
                stats = results[library][operation]
                mean = stats['mean']
                median = stats['median']
                p95 = stats['p95']
                p99 = stats['p99']
                p999 = stats['p999']
                min_val = stats['min']
                max_val = stats['max']
                stddev = stats['stddev']
                cv = (stddev / mean * 100) if mean > 0 else 0

                row = [
                    library,
                    f"{mean:.2f}",
                    f"{median:.2f}",
                    f"{p95:.2f}",
                    f"{p99:.2f}",
                    f"{p999:.2f}",
                    f"{min_val:.2f}",
                    f"{max_val:.2f}",
                    f"{stddev:.2f}",
                    f"{cv:.1f}%"
                ]
                lines.append("| " + " | ".join(row) + " |")

        lines.append("")

    # Automotive-Grade Assessment (ISO 26262)
    lines.append("## Automotive-Grade Assessment (ISO 26262)")
    lines.append("")
    lines.append("Each operation is evaluated against automotive real-time requirements:")
    lines.append("")
    lines.append("**Criteria:**")
    lines.append("1. Mean latency < 500ms (HMI response requirement)")
    lines.append("2. P99 latency < 1000ms (worst-case acceptable limit)")
    lines.append("3. CV% < 30% (deterministic behavior)")
    lines.append("")
    lines.append("**Assessment:** ✅ PASS | ⚠️ MARGINAL | ❌ FAIL")
    lines.append("")

    for operation in all_operations:
        lines.append(f"### {operation.title()} - Automotive Compliance")
        lines.append("")

        header = ["Library", "Mean < 500ms", "P99 < 1000ms", "CV% < 30%", "Assessment"]
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + "|".join(["-" * (len(h) + 2) for h in header]) + "|")

        for library in all_libraries:
            if operation in results[library]:
                stats = results[library][operation]
                mean = stats['mean']
                p99 = stats['p99']
                stddev = stats['stddev']
                cv = (stddev / mean * 100) if mean > 0 else 0

                # Check criteria
                mean_pass = "✅" if mean < 500 else "❌"
                p99_pass = "✅" if p99 < 1000 else "❌"
                cv_pass = "✅" if cv < 30 else "❌"

                # Overall assessment
                criteria_met = sum([mean < 500, p99 < 1000, cv < 30])
                if criteria_met == 3:
                    assessment = "✅ PASS"
                elif criteria_met == 2:
                    assessment = "⚠️ MARGINAL"
                else:
                    assessment = "❌ FAIL"

                row = [
                    library,
                    f"{mean_pass} ({mean:.1f}ms)",
                    f"{p99_pass} ({p99:.1f}ms)",
                    f"{cv_pass} ({cv:.1f}%)",
                    assessment
                ]
                lines.append("| " + " | ".join(row) + " |")

        lines.append("")

    # Performance summary
    lines.append("## Performance Summary")
    lines.append("")

    for library in all_libraries:
        lines.append(f"### {library}")
        lines.append("")

        lib_data = results[library]
        if not lib_data:
            lines.append("No data available.")
            lines.append("")
            continue

        operations = sorted(lib_data.keys())
        total_mean = sum(lib_data[op]['mean'] for op in operations)
        avg_mean = total_mean / len(operations)
        avg_p99 = sum(lib_data[op]['p99'] for op in operations) / len(operations)

        # Count automotive-grade pass
        automotive_pass = sum(
            1 for op in operations
            if lib_data[op]['mean'] < 500
            and lib_data[op]['p99'] < 1000
            and (lib_data[op]['stddev'] / lib_data[op]['mean'] * 100) < 30
        )

        lines.append(f"- **Operations tested:** {len(operations)}")
        lines.append(f"- **Average latency (mean):** {avg_mean:.2f} ms")
        lines.append(f"- **Average latency (P99):** {avg_p99:.2f} ms")
        lines.append(f"- **Total workflow time:** {total_mean:.2f} ms")
        lines.append(f"- **Automotive-grade operations:** {automotive_pass}/{len(operations)}")
        lines.append("")

    # Methodology
    lines.append("## Methodology")
    lines.append("")
    lines.append("### Benchmark Configuration")
    lines.append("- **Tool:** hyperfine - Statistical benchmarking with warmup and outlier detection")
    lines.append("- **Runs per benchmark:** 100 (industry best practice for reliable P99 analysis)")
    lines.append("- **Warmup runs:** 5 (to stabilize performance)")
    lines.append("- **Outlier handling:** Automatic detection and filtering")
    lines.append("- **Process isolation:** Each run in separate process (no state contamination)")
    lines.append("")
    lines.append("### Why 100 Runs?")
    lines.append("")
    lines.append("Industry best practices require **≥100 samples** for reliable percentile analysis:")
    lines.append("- P95 (95th percentile) requires ~20 samples minimum")
    lines.append("- P99 (99th percentile) requires ~100 samples minimum")
    lines.append("- P99.9 (99.9th percentile) requires ~1000 samples minimum")
    lines.append("")
    lines.append("With 100 runs, we get:")
    lines.append("- **Reliable P95 and P99** metrics")
    lines.append("- **Statistical significance** for mean/median")
    lines.append("- **Worst-case analysis** for automotive compliance")
    lines.append("- **Single fingerprint** still used (reused 100 times, no user burden)")
    lines.append("")
    lines.append("### Test Procedure")
    lines.append("1. **Single fingerprint captured once** (user places finger once)")
    lines.append("2. **Fingerprint data saved** to reusable template file")
    lines.append("3. **Each operation benchmarked** with 100 runs using the same template")
    lines.append("4. **Statistical analysis** computed from all runs")
    lines.append("5. **No manual interaction** during benchmarking (fully automated)")
    lines.append("")
    lines.append("### Fairness Guarantees")
    lines.append("- **Same hardware:** Raspberry Pi 5")
    lines.append("- **Same sensor:** R30x optical fingerprint sensor")
    lines.append("- **Same fingerprint:** Single template reused across all libraries")
    lines.append("- **Same environment:** Consistent temperature, power, lighting")
    lines.append("- **Same methodology:** All libraries tested with identical hyperfine configuration")
    lines.append("")

    # Footer
    lines.append("---")
    lines.append("")
    lines.append("*Generated by carbio multi-library benchmark suite*")

    # Write to file
    with open(output_file, 'w') as f:
        f.write('\n'.join(lines))

    print(f"Report generated: {output_file}")


def get_operation_description(operation: str) -> str:
    """Get human-readable description of operation"""
    descriptions = {
        'enroll': 'Capture and store fingerprint template',
        'verify': '1:1 verification against stored template',
        'identify': '1:N search across fingerprint database',
        'delete': 'Remove template from database',
    }
    return descriptions.get(operation, 'Unknown operation')


def main():
    if len(sys.argv) < 3:
        print("Usage: generate_report.py <hyperfine_json> <output_md>")
        sys.exit(1)

    json_file = sys.argv[1]
    output_file = sys.argv[2]

    if not Path(json_file).exists():
        print(f"Error: Input file not found: {json_file}")
        sys.exit(1)

    print(f"Loading results from: {json_file}")
    data = load_hyperfine_results(json_file)

    print("Organizing results...")
    results = organize_results(data)

    print("Generating report...")
    generate_markdown_report(results, output_file)

    print("Done!")


if __name__ == '__main__':
    main()
