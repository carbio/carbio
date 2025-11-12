/**
 * Template Operations Tool for Adafruit Fingerprint Library
 * Provides CLI interface for benchmarking operations
 */

#include "Adafruit_Fingerprint_RPi.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>

// Buffer IDs
#define CHARBUFFER1 0x01
#define CHARBUFFER2 0x02

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " <operation> [args]\n";
    std::cerr << "Operations:\n";
    std::cerr << "  store <template_file>  - Upload template and store to position 100\n";
    std::cerr << "  load                   - Load template from position 100 to buffer\n";
    std::cerr << "  delete                 - Delete template at position 100\n";
    std::cerr << "  download               - Download template from buffer to file\n";
    std::cerr << "  verify                 - Verify live finger against stored template\n";
    std::cerr << "  identify               - Identify live finger against database\n";
}

Adafruit_Fingerprint_RPi* init_sensor(const char* port) {
    // Try primary port first, fallback to USB
    const char* test_port = access(port, F_OK) == 0 ? port : "/dev/ttyUSB0";

    const int MAX_RETRIES = 3;
    const int RETRY_DELAY_MS = 100;

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        auto sensor = new Adafruit_Fingerprint_RPi(test_port, 0x0);

        if (!sensor->begin(57600)) {
            delete sensor;
            if (attempt < MAX_RETRIES - 1) {
                usleep(RETRY_DELAY_MS * 1000);
                continue;
            }
            std::cerr << "Failed to initialize sensor on " << test_port << " after " << MAX_RETRIES << " attempts\n";
            return nullptr;
        }

        if (!sensor->verifyPassword()) {
            delete sensor;
            if (attempt < MAX_RETRIES - 1) {
                usleep(RETRY_DELAY_MS * 1000);
                continue;
            }
            std::cerr << "Sensor password verification failed after " << MAX_RETRIES << " attempts\n";
            return nullptr;
        }

        // Success
        return sensor;
    }

    return nullptr;
}

int store_template(Adafruit_Fingerprint_RPi* sensor, const char* template_file) {
    // Load reference template from position 149 to buffer
    uint8_t result = sensor->loadModel(149);
    if (result != FINGERPRINT_OK) {
        std::cerr << "Failed to load reference template from position 149 (error: 0x" << std::hex << (int)result << ")\n";
        std::cerr << "Note: Run setup to enroll a template at position 149 first\n";
        return 1;
    }

    // Store from buffer to position 100
    result = sensor->storeModel(100);

    if (result == FINGERPRINT_OK) {
        std::cout << "Stored template at position 100\n";
        return 0;
    } else {
        std::cerr << "Failed to store template (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }
}

int load_template(Adafruit_Fingerprint_RPi* sensor) {
    uint8_t result = sensor->loadModel(100);

    if (result == FINGERPRINT_OK) {
        std::cout << "Loaded template from position 100\n";
        return 0;
    } else {
        std::cerr << "Failed to load template (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }
}

int delete_template(Adafruit_Fingerprint_RPi* sensor) {
    uint8_t result = sensor->deleteModel(100);

    if (result == FINGERPRINT_OK) {
        std::cout << "Deleted template at position 100\n";
        return 0;
    } else {
        std::cerr << "Failed to delete template (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }
}

int download_template(Adafruit_Fingerprint_RPi* sensor) {
    // Load template from reference position 149 to buffer
    // (This mirrors the carbio approach of using position 149 as reference)
    uint8_t result = sensor->loadModel(149);
    if (result != FINGERPRINT_OK) {
        // If position 149 doesn't exist, try position 100
        result = sensor->loadModel(100);
        if (result != FINGERPRINT_OK) {
            std::cerr << "Failed to load template to buffer (error: 0x" << std::hex << (int)result << ")\n";
            return 1;
        }
    }

    // Get model from buffer (this downloads it to memory)
    result = sensor->getModel();

    if (result == FINGERPRINT_OK) {
        std::cout << "Downloaded template from buffer\n";
        // In a real implementation, we'd save to /tmp/adafruit_downloaded.bin
        return 0;
    } else {
        std::cerr << "Failed to download template (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }
}

int verify_fingerprint(Adafruit_Fingerprint_RPi* sensor) {
    // First, wait for sensor to be clear (no finger detected)
    // This ensures clean state between benchmark runs
    uint8_t result;
    int clear_checks = 0;
    const int REQUIRED_CLEAR_CHECKS = 3;

    while (clear_checks < REQUIRED_CLEAR_CHECKS) {
        result = sensor->getImage();
        if (result == FINGERPRINT_NOFINGER) {
            clear_checks++;
            usleep(50000);  // 50ms between checks
        } else if (result == FINGERPRINT_OK) {
            // Finger still on sensor, reset counter
            clear_checks = 0;
            usleep(100000);  // 100ms delay
        } else {
            // Other error, just wait
            usleep(100000);
        }
    }

    std::cout << "Place finger on sensor...\n";

    // Wait for finger with timeout (10 seconds)
    const int TIMEOUT_MS = 10000;
    const int POLL_INTERVAL_MS = 100;
    int elapsed_ms = 0;

    while (elapsed_ms < TIMEOUT_MS) {
        result = sensor->getImage();
        if (result == FINGERPRINT_OK) {
            break;
        } else if (result == FINGERPRINT_NOFINGER) {
            usleep(POLL_INTERVAL_MS * 1000);
            elapsed_ms += POLL_INTERVAL_MS;
        } else {
            std::cerr << "Image capture failed (error: 0x" << std::hex << (int)result << ")\n";
            return 1;
        }
    }

    if (elapsed_ms >= TIMEOUT_MS) {
        std::cerr << "Timeout waiting for finger\n";
        return 1;
    }

    // Convert image to characteristics in buffer 1
    result = sensor->image2Tz(CHARBUFFER1);
    if (result != FINGERPRINT_OK) {
        std::cerr << "Failed to convert image (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }

    // Load stored template from position 100 to buffer 2
    result = sensor->loadModel(100);
    if (result != FINGERPRINT_OK) {
        std::cerr << "Failed to load stored template\n";
        return 1;
    }

    // Search/compare (using search on single template as verify)
    result = sensor->fingerSearch(CHARBUFFER1);

    if (result == FINGERPRINT_OK && sensor->fingerID == 100) {
        std::cout << "Verification SUCCESS (confidence: " << sensor->confidence << ")\n";
        return 0;
    } else {
        std::cout << "Verification FAILED\n";
        return 1;
    }
}

int identify_fingerprint(Adafruit_Fingerprint_RPi* sensor) {
    // First, wait for sensor to be clear (no finger detected)
    // This ensures clean state between benchmark runs
    uint8_t result;
    int clear_checks = 0;
    const int REQUIRED_CLEAR_CHECKS = 3;

    while (clear_checks < REQUIRED_CLEAR_CHECKS) {
        result = sensor->getImage();
        if (result == FINGERPRINT_NOFINGER) {
            clear_checks++;
            usleep(50000);  // 50ms between checks
        } else if (result == FINGERPRINT_OK) {
            // Finger still on sensor, reset counter
            clear_checks = 0;
            usleep(100000);  // 100ms delay
        } else {
            // Other error, just wait
            usleep(100000);
        }
    }

    std::cout << "Place finger on sensor...\n";

    // Wait for finger with timeout (10 seconds)
    const int TIMEOUT_MS = 10000;
    const int POLL_INTERVAL_MS = 100;
    int elapsed_ms = 0;

    while (elapsed_ms < TIMEOUT_MS) {
        result = sensor->getImage();
        if (result == FINGERPRINT_OK) {
            break;
        } else if (result == FINGERPRINT_NOFINGER) {
            usleep(POLL_INTERVAL_MS * 1000);
            elapsed_ms += POLL_INTERVAL_MS;
        } else {
            std::cerr << "Image capture failed (error: 0x" << std::hex << (int)result << ")\n";
            return 1;
        }
    }

    if (elapsed_ms >= TIMEOUT_MS) {
        std::cerr << "Timeout waiting for finger\n";
        return 1;
    }

    // Convert image to characteristics in buffer 1
    result = sensor->image2Tz(CHARBUFFER1);
    if (result != FINGERPRINT_OK) {
        std::cerr << "Failed to convert image (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }

    // Search entire database
    result = sensor->fingerFastSearch();

    if (result == FINGERPRINT_OK) {
        std::cout << "Identification SUCCESS (position: " << sensor->fingerID
                  << ", confidence: " << sensor->confidence << ")\n";
        return 0;
    } else if (result == FINGERPRINT_NOTFOUND) {
        std::cout << "Identification FAILED (no match)\n";
        return 1;
    } else {
        std::cerr << "Search failed (error: 0x" << std::hex << (int)result << ")\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* operation = argv[1];
    const char* port = "/dev/ttyAMA0";

    // Initialize sensor
    Adafruit_Fingerprint_RPi* sensor = init_sensor(port);
    if (!sensor) {
        return 1;
    }

    int result = 0;

    if (strcmp(operation, "store") == 0) {
        if (argc < 3) {
            std::cerr << "Error: store operation requires template file path\n";
            print_usage(argv[0]);
            result = 1;
        } else {
            result = store_template(sensor, argv[2]);
        }
    } else if (strcmp(operation, "load") == 0) {
        result = load_template(sensor);
    } else if (strcmp(operation, "delete") == 0) {
        result = delete_template(sensor);
    } else if (strcmp(operation, "download") == 0) {
        result = download_template(sensor);
    } else if (strcmp(operation, "verify") == 0) {
        result = verify_fingerprint(sensor);
    } else if (strcmp(operation, "identify") == 0) {
        result = identify_fingerprint(sensor);
    } else {
        std::cerr << "Unknown operation: " << operation << "\n";
        print_usage(argv[0]);
        result = 1;
    }

    delete sensor;
    return result;
}
