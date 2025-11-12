/**
 * Performance Benchmark for Adafruit Fingerprint Sensor Library (Arduino Port to RPi)
 * Measures real physical timing with actual R30x sensor
 *
 * Operations tested:
 * - Image capture
 * - Feature extraction
 * - Template creation
 * - Template storage
 * - Template search
 * - Template load
 * - Template delete
 * - Get template count
 */

#include "Adafruit_Fingerprint_RPi.h"
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>
#include <unistd.h>

using namespace std;
using namespace std::chrono;

struct BenchmarkStats {
    string operation;
    int count;
    double mean;
    double median;
    double min_val;
    double max_val;
    double std_dev;
    double p95;
    double p99;
};

class BenchmarkResults {
private:
    map<string, vector<double>> measurements;

public:
    void addMeasurement(const string& operation, double duration_ms) {
        measurements[operation].push_back(duration_ms);
    }

    BenchmarkStats getStats(const string& operation) {
        BenchmarkStats stats;
        stats.operation = operation;

        if (measurements.find(operation) == measurements.end() || measurements[operation].empty()) {
            stats.count = 0;
            return stats;
        }

        vector<double> data = measurements[operation];
        sort(data.begin(), data.end());
        int n = data.size();

        stats.count = n;
        stats.mean = accumulate(data.begin(), data.end(), 0.0) / n;
        stats.median = data[n / 2];
        stats.min_val = data.front();
        stats.max_val = data.back();

        // Standard deviation
        double variance = 0.0;
        for (double val : data) {
            variance += (val - stats.mean) * (val - stats.mean);
        }
        stats.std_dev = sqrt(variance / (n - 1));

        stats.p95 = data[static_cast<size_t>(n * 0.95)];
        stats.p99 = data[static_cast<size_t>(n * 0.99)];

        return stats;
    }

    void printSummary() {
        cout << "\nBENCHMARK RESULTS - Adafruit Arduino Library (Raspberry Pi Port)\n\n";

        for (const auto& pair : measurements) {
            BenchmarkStats stats = getStats(pair.first);
            if (stats.count > 0) {
                cout << stats.operation << ":\n";
                cout << "  Iterations: " << stats.count << "\n";
                cout << "  Mean:       " << fixed << setprecision(2) << stats.mean << " ms\n";
                cout << "  Median:     " << stats.median << " ms\n";
                cout << "  Min:        " << stats.min_val << " ms\n";
                cout << "  Max:        " << stats.max_val << " ms\n";
                cout << "  Std Dev:    " << stats.std_dev << " ms\n";
                cout << "  P95:        " << stats.p95 << " ms\n";
                cout << "  P99:        " << stats.p99 << " ms\n\n";
            }
        }
    }

    void saveJSON(const string& filename) {
        ofstream file(filename);
        file << "{\n";

        bool first_op = true;
        for (const auto& pair : measurements) {
            BenchmarkStats stats = getStats(pair.first);
            if (stats.count > 0) {
                if (!first_op) file << ",\n";
                first_op = false;

                file << "  \"" << stats.operation << "\": {\n";
                file << "    \"count\": " << stats.count << ",\n";
                file << "    \"mean\": " << stats.mean << ",\n";
                file << "    \"median\": " << stats.median << ",\n";
                file << "    \"min\": " << stats.min_val << ",\n";
                file << "    \"max\": " << stats.max_val << ",\n";
                file << "    \"std_dev\": " << stats.std_dev << ",\n";
                file << "    \"p95\": " << stats.p95 << ",\n";
                file << "    \"p99\": " << stats.p99 << "\n";
                file << "  }";
            }
        }

        file << "\n}\n";
        file.close();
        cout << "\nResults saved to: " << filename << "\n";
    }
};

// Real-time visual feedback helpers
void printSeparator() {
    cout << "────────────────────────────────────────────────────────────────" << endl;
}

void printStatus(const string& message, bool newline = true) {
    cout << "  ▶ " << message << flush;
    if (newline) cout << endl;
}

void printSuccess(const string& message) {
    cout << "  ✓ " << message << endl;
}

void printError(const string& message) {
    cout << "  ✗ " << message << endl;
}

// Wait for finger with real-time visual feedback and countdown
bool waitForFinger(Adafruit_Fingerprint_RPi& finger, int timeout_seconds = 15) {
    printStatus("Waiting for finger... (" + to_string(timeout_seconds) + " seconds)", false);

    auto start = high_resolution_clock::now();
    int last_second = -1;
    int dots = 0;

    while (true) {
        auto now = high_resolution_clock::now();
        int elapsed = duration_cast<seconds>(now - start).count();
        int remaining = timeout_seconds - elapsed;

        if (elapsed >= timeout_seconds) {
            cout << "\r  ✗ TIMEOUT: No finger detected after " << timeout_seconds << " seconds           " << endl;
            return false;
        }

        // Update countdown every second
        if (elapsed != last_second) {
            last_second = elapsed;
            cout << "\r  ▶ Waiting for finger... (" << remaining << "s remaining)" << flush;
            dots = 0;
        }

        // Check for finger
        uint8_t response = finger.getImage();
        if (response == FINGERPRINT_OK) {
            cout << "\r  ✓ Finger detected!                                              " << endl;
            return true;
        }

        // Animate dots
        usleep(100000);  // 100ms
        dots++;
        if (dots % 3 == 0) {
            cout << "." << flush;
        }
    }
}

// Wait for finger removal with visual feedback
void waitForFingerRemoval(Adafruit_Fingerprint_RPi& finger) {
    printStatus("Remove finger...", false);

    int dots = 0;
    while (finger.getImage() != FINGERPRINT_NOFINGER) {
        usleep(100000);  // 100ms
        dots++;
        if (dots % 3 == 0) {
            cout << "." << flush;
        }
    }
    cout << "\r  ✓ Finger removed                    " << endl;
}

void benchmarkImageCapture(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 3) {
    printSeparator();
    cout << "[1/8] Benchmarking Image Capture (" << iterations << " iterations)" << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }

        auto start = high_resolution_clock::now();
        uint8_t response = finger.getImage();
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("image_capture", duration_ms);
            printSuccess("Captured in " + to_string(duration_ms) + " ms");
        } else {
            printError("Failed with response code " + to_string((int)response));
        }

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkFeatureExtraction(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 3) {
    printSeparator();
    cout << "[2/8] Benchmarking Feature Extraction (" << iterations << " iterations)" << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }

        // Measure feature extraction
        auto start = high_resolution_clock::now();
        uint8_t response = finger.image2Tz(1);
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("feature_extraction", duration_ms);
            printSuccess("Extracted in " + to_string(duration_ms) + " ms");
        } else {
            printError("Failed with response code " + to_string((int)response));
        }

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkTemplateCreation(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 3) {
    printSeparator();
    cout << "[3/8] Benchmarking Template Creation (" << iterations << " iterations)" << endl;
    cout << "    NOTE: Requires 2 finger placements per iteration" << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        // First sample
        printStatus("Sample 1/2:");
        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }
        finger.image2Tz(1);
        printSuccess("Sample 1/2 captured");
        waitForFingerRemoval(finger);

        // Second sample
        printStatus("Sample 2/2:");
        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }
        finger.image2Tz(2);
        printSuccess("Sample 2/2 captured");

        // Measure template creation
        auto start = high_resolution_clock::now();
        uint8_t response = finger.createModel();
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("template_creation", duration_ms);
            printSuccess("Template created in " + to_string(duration_ms) + " ms");
        } else {
            printError("Failed with response code " + to_string((int)response));
        }

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkTemplateStorage(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, uint16_t template_id, int iterations = 3) {
    printSeparator();
    cout << "[4/8] Benchmarking Template Storage (" << iterations << " iterations)" << endl;
    cout << "    NOTE: Requires 2 finger placements per iteration + storage to ID " << template_id << endl;
    printSeparator();

    const uint16_t TEST_ID = template_id;

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        // First sample
        printStatus("Sample 1/2 for template creation:");
        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }
        finger.image2Tz(1);
        printSuccess("Sample 1/2 captured");
        waitForFingerRemoval(finger);

        // Second sample
        printStatus("Sample 2/2 for template creation:");
        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }
        finger.image2Tz(2);
        printSuccess("Sample 2/2 captured");
        finger.createModel();
        printSuccess("Template created");

        // Measure storage
        auto start = high_resolution_clock::now();
        uint8_t response = finger.storeModel(TEST_ID);
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("template_storage", duration_ms);
            printSuccess("Template stored in " + to_string(duration_ms) + " ms");
        } else {
            printError("Failed with response code " + to_string((int)response));
        }

        // NOTE: Don't delete template immediately - keep it for search benchmarks
        // Templates will be cleaned up at the end of the benchmark suite

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkTemplateSearch(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 5) {
    printSeparator();
    cout << "[5/8] Benchmarking Template Search (" << iterations << " iterations)" << endl;
    cout << "    NOTE: Requires at least one enrolled fingerprint in database" << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }

        finger.image2Tz(1);

        auto start = high_resolution_clock::now();
        uint8_t response = finger.fingerSearch(1);
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("template_search", duration_ms);
            printSuccess("Found ID #" + to_string(finger.fingerID) + ", confidence " +
                        to_string(finger.confidence) + " in " + to_string(duration_ms) + " ms");
        } else {
            printError("Not found or search error");
        }

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkFastSearch(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 5) {
    printSeparator();
    cout << "[6/8] Benchmarking Fast Search (" << iterations << " iterations)" << endl;
    cout << "    NOTE: Requires at least one enrolled fingerprint in database" << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Iteration " << (i + 1) << "/" << iterations << ":" << endl;

        if (!waitForFinger(finger, 15)) {
            printError("Skipping iteration due to timeout");
            continue;
        }

        finger.image2Tz(1);

        auto start = high_resolution_clock::now();
        uint8_t response = finger.fingerFastSearch();
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("template_fast_search", duration_ms);
            printSuccess("Found ID #" + to_string(finger.fingerID) + ", confidence " +
                        to_string(finger.confidence) + " in " + to_string(duration_ms) + " ms");
        } else {
            printError("Not found or search error");
        }

        waitForFingerRemoval(finger);
    }

    cout << endl;
}

void benchmarkTemplateDelete(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, uint16_t start_id, int iterations = 5) {
    printSeparator();
    cout << "[7/8] Benchmarking Template Deletion (" << iterations << " iterations)" << endl;
    cout << "    NOTE: First enrolls " << iterations << " test templates, then benchmarks deletion" << endl;
    printSeparator();

    const uint16_t TEST_ID_START = start_id;

    // Enroll test templates
    cout << "\n  PREPARATION PHASE: Enrolling " << iterations << " test templates..." << endl;
    printSeparator();

    for (int i = 0; i < iterations; i++) {
        cout << "\n  Enrolling template " << (i + 1) << "/" << iterations << " (ID " << (TEST_ID_START + i) << "):" << endl;

        // First sample
        printStatus("Sample 1/2:");
        if (!waitForFinger(finger, 15)) {
            printError("Enrollment failed - retrying this template");
            i--;  // Retry this template
            continue;
        }
        finger.image2Tz(1);
        printSuccess("Sample 1/2 captured");
        waitForFingerRemoval(finger);

        // Second sample
        printStatus("Sample 2/2:");
        if (!waitForFinger(finger, 15)) {
            printError("Enrollment failed - retrying this template");
            i--;  // Retry this template
            continue;
        }
        finger.image2Tz(2);
        printSuccess("Sample 2/2 captured");
        finger.createModel();
        finger.storeModel(TEST_ID_START + i);
        printSuccess("Template enrolled to ID " + to_string(TEST_ID_START + i));
        waitForFingerRemoval(finger);
    }

    // Benchmark deletion
    cout << "\n";
    printSeparator();
    cout << "  DELETION PHASE: Benchmarking deletion of " << iterations << " templates..." << endl;
    printSeparator();
    cout << endl;

    for (int i = 0; i < iterations; i++) {
        auto start = high_resolution_clock::now();
        uint8_t response = finger.deleteModel(TEST_ID_START + i);
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("template_delete", duration_ms);
            printSuccess("Iteration " + to_string(i + 1) + "/" + to_string(iterations) +
                        ": Deleted ID " + to_string(TEST_ID_START + i) + " in " + to_string(duration_ms) + " ms");
        } else {
            printError("Iteration " + to_string(i + 1) + "/" + to_string(iterations) + ": Failed to delete ID " + to_string(TEST_ID_START + i));
        }
    }

    cout << endl;
}

void benchmarkGetTemplateCount(Adafruit_Fingerprint_RPi& finger, BenchmarkResults& results, int iterations = 100) {
    printSeparator();
    cout << "[8/8] Benchmarking Get Template Count (" << iterations << " iterations)" << endl;
    cout << "    NOTE: Automated test - no manual interaction required" << endl;
    printSeparator();

    printStatus("Running " + to_string(iterations) + " automated queries...");
    cout << endl;

    int success_count = 0;
    for (int i = 0; i < iterations; i++) {
        auto start = high_resolution_clock::now();
        uint8_t response = finger.getTemplateCount();
        auto end = high_resolution_clock::now();

        if (response == FINGERPRINT_OK) {
            double duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            results.addMeasurement("get_template_count", duration_ms);
            success_count++;

            // Show first 3, last, and progress every 25 iterations
            if (i < 3 || i == iterations - 1 || (i + 1) % 25 == 0) {
                printSuccess("Iteration " + to_string(i + 1) + "/" + to_string(iterations) + ": " +
                            to_string(duration_ms) + " ms (count: " + to_string(finger.templateCount) + ")");
            }
        } else {
            printError("Iteration " + to_string(i + 1) + "/" + to_string(iterations) + ": Failed");
        }

        usleep(10000); // 10ms delay
    }

    cout << endl;
    printSeparator();
    printSuccess("Completed " + to_string(success_count) + "/" + to_string(iterations) + " iterations successfully");
    printSeparator();
    cout << endl;
}

int main() {
    // CRITICAL: Configure C++ streams for REAL-TIME unbuffered output
    cout.setf(ios::unitbuf);  // Unbuffered cout
    cerr.setf(ios::unitbuf);  // Unbuffered cerr
    setvbuf(stdout, nullptr, _IONBF, 0);  // Force unbuffered stdout at C level
    setvbuf(stderr, nullptr, _IONBF, 0);  // Force unbuffered stderr at C level

    cout << "\nPerformance Benchmark: Adafruit Arduino Library (Raspberry Pi Port)\n\n";

    cout << "This benchmark measures REAL physical timing with actual R30x sensor\n";
    cout << "You will be prompted to place/remove finger for each measurement\n\n";

    // Try different ports
    const char* ports[] = {"/dev/ttyAMA0", "/dev/ttyUSB0", nullptr};
    Adafruit_Fingerprint_RPi* finger = nullptr;

    for (int i = 0; ports[i] != nullptr; i++) {
        cout << "Trying port: " << ports[i] << "\n";
        finger = new Adafruit_Fingerprint_RPi(ports[i]);
        if (finger->begin(57600)) {
            cout << "SUCCESS: Connected on " << ports[i] << "\n";
            break;
        }
        delete finger;
        finger = nullptr;
    }

    if (!finger) {
        cerr << "\nERROR: Could not connect to sensor on any port!\n";
        return 1;
    }

    if (!finger->verifyPassword()) {
        cerr << "ERROR: Did not find fingerprint sensor!\n";
        delete finger;
        return 1;
    }

    cout << "SUCCESS: Fingerprint sensor verified\n";

    // CRITICAL: Check sensor capacity to avoid ID out-of-range errors
    finger->getParameters();
    uint16_t sensor_capacity = finger->capacity;
    cout << "Sensor capacity: " << sensor_capacity << " templates (IDs 0-" << (sensor_capacity - 1) << ")\n";

    if (sensor_capacity < 50) {
        cerr << "\nERROR: Sensor capacity too small (" << sensor_capacity << ") for benchmarks!\n";
        delete finger;
        return 1;
    }

    // Use safe template IDs within sensor capacity
    // Place them near the end to avoid conflicts with real enrollments
    uint16_t safe_id_1 = sensor_capacity - 50;  // e.g., 100 for 150-capacity sensor
    uint16_t safe_id_2 = sensor_capacity - 30;  // e.g., 120 for 150-capacity sensor

    cout << "Using safe template IDs: " << safe_id_1 << " and " << safe_id_2 << "\n\n";

    printSeparator();
    printStatus("Starting benchmarks in 0.5 seconds...");
    printSeparator();
    usleep(500000);  // 0.5 second delay before auto-start
    cout << endl;

    BenchmarkResults results;

    // Run benchmarks
    benchmarkImageCapture(*finger, results);
    benchmarkFeatureExtraction(*finger, results);
    benchmarkTemplateCreation(*finger, results);
    benchmarkTemplateStorage(*finger, results, safe_id_1);
    benchmarkTemplateSearch(*finger, results);
    benchmarkFastSearch(*finger, results);
    benchmarkGetTemplateCount(*finger, results);
    benchmarkTemplateDelete(*finger, results, safe_id_2);

    // Print summary
    results.printSummary();

    // Save results
    results.saveJSON("adafruit_arduino_results.json");

    cout << "\nBenchmark complete!\n";

    cout << "Cleaning up test templates...\n";
    finger->deleteModel(safe_id_1);
    for (int i = 0; i < 5; i++) {
        finger->deleteModel(safe_id_2 + i);
    }
    cout << "Cleanup complete.\n";

    delete finger;
    return 0;
}
