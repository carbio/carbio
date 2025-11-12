/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * CLI Tool  : carbiok - Command-line interface for carbio library
 *********************************************************************/

#include "fingerprint/fingerprint_sensor.h"

#include <gflags/gflags.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace carbio;

// ============================================================================
// Command-line Flags
// ============================================================================

// Connection
DEFINE_string(port, "/dev/ttyAMA0", "Serial port path");

// Common parameters
DEFINE_uint32(id, 0, "Template ID/position");
DEFINE_uint32(buffer, 1, "Buffer ID (1 or 2)");
DEFINE_uint32(count, 0, "Count parameter (0 = all)");
DEFINE_uint32(page, 0, "Page ID for search start");
DEFINE_uint32(samples, 12, "Sample count for enrollment");
DEFINE_string(file, "", "File path for upload/download operations");
DEFINE_uint32(timeout, 2000, "Timeout in milliseconds for verify/identify operations (default: 2000)");

// Low-level commands
DEFINE_bool(capture, false, "Capture fingerprint image");
DEFINE_bool(extract, false, "Extract features from image to buffer");
DEFINE_bool(merge, false, "Merge two character buffers into template");
DEFINE_bool(store, false, "Store template from buffer to flash");
DEFINE_bool(load, false, "Load template from flash to buffer");
DEFINE_bool(down, false, "Download template from buffer to file");
DEFINE_bool(up, false, "Upload template from file to buffer");
DEFINE_bool(erase, false, "Erase template(s) from flash");
DEFINE_bool(erase_all, false, "Erase all templates in database");
DEFINE_bool(match, false, "Match two buffers (1:1)");
DEFINE_bool(search, false, "Search template in database (1:N)");
DEFINE_bool(fast_search, false, "Fast search template in database");
DEFINE_bool(get_count, false, "Get template count in database");

// Template file operations
DEFINE_bool(export, false, "Export template from flash position to file");
DEFINE_bool(import, false, "Import template from file to flash position");

// Utility commands
DEFINE_int32(led, -1, "Turn LED off (0) or on (1)");

// High-level commands
DEFINE_bool(enroll, false, "Enroll new fingerprint (capture + store)");
DEFINE_bool(verify, false, "Verify fingerprint against ID");
DEFINE_bool(identify, false, "Identify fingerprint (search database)");

// ============================================================================
// Utility Functions
// ============================================================================

std::optional<std::vector<std::uint8_t>> read_file(const std::string& path)
{
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
  {
    std::cerr << "Error: Cannot open file: " << path << std::endl;
    return std::nullopt;
  }

  auto size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
  {
    std::cerr << "Error: Failed to read file: " << path << std::endl;
    return std::nullopt;
  }

  return buffer;
}

bool write_file(const std::string& path, std::span<const std::uint8_t> data)
{
  std::ofstream file(path, std::ios::binary);
  if (!file)
  {
    std::cerr << "Error: Cannot create file: " << path << std::endl;
    return false;
  }

  if (!file.write(reinterpret_cast<const char*>(data.data()), data.size()))
  {
    std::cerr << "Error: Failed to write file: " << path << std::endl;
    return false;
  }

  return true;
}

template<typename T>
void print_result(const result<T>& r, const char* operation)
{
  if (r)
  {
    std::cout << "SUCCESS: " << operation << std::endl;
  }
  else
  {
    std::cerr << "FAILED: " << operation << " - " << message(r.error()) << std::endl;
  }
}

void print_result(const void_result& r, const char* operation)
{
  if (r)
  {
    std::cout << "SUCCESS: " << operation << std::endl;
  }
  else
  {
    std::cerr << "FAILED: " << operation << " - " << message(r.error()) << std::endl;
  }
}

// ============================================================================
// Command Implementations
// ============================================================================

int cmd_capture(fingerprint_sensor& sensor)
{
  std::cout << "Capturing fingerprint image..." << std::endl;
  std::cout << "Place finger on sensor..." << std::endl;

  auto result = sensor.capture_image();
  print_result(result, "Capture image");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_extract(fingerprint_sensor& sensor)
{
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);
  std::cout << "Extracting features to buffer " << static_cast<int>(buffer_id) << "..." << std::endl;

  auto result = sensor.extract_features(buffer_id);
  print_result(result, "Extract features");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_merge(fingerprint_sensor& sensor)
{
  std::cout << "Merging buffers 1 and 2 into template model..." << std::endl;

  auto result = sensor.merge_model();
  print_result(result, "Merge model");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_store(fingerprint_sensor& sensor)
{
  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Storing template from buffer " << static_cast<int>(buffer_id)
            << " to position " << id << "..." << std::endl;

  auto result = sensor.store_model(id, buffer_id);
  print_result(result, "Store model");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_load(fingerprint_sensor& sensor)
{
  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Loading template from position " << id
            << " to buffer " << static_cast<int>(buffer_id) << "..." << std::endl;

  auto result = sensor.load_model(id, buffer_id);
  print_result(result, "Load model");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_download(fingerprint_sensor& sensor)
{
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Downloading template from buffer " << static_cast<int>(buffer_id) << "..." << std::endl;

  auto result = sensor.download_model(buffer_id);
  if (!result)
  {
    print_result(result, "Download model");
    return EXIT_FAILURE;
  }

  std::cout << "Template size: " << result.value().size() << " bytes" << std::endl;

  if (!FLAGS_file.empty())
  {
    if (write_file(FLAGS_file, result.value()))
    {
      std::cout << "Saved to: " << FLAGS_file << std::endl;
      return EXIT_SUCCESS;
    }
    else
    {
      return EXIT_FAILURE;
    }
  }
  else
  {
    std::cout << "SUCCESS: Download model (use --file to save)" << std::endl;
    return EXIT_SUCCESS;
  }
}

int cmd_upload(fingerprint_sensor& sensor)
{
  if (FLAGS_file.empty())
  {
    std::cerr << "Error: --file required for upload" << std::endl;
    return EXIT_FAILURE;
  }

  auto data = read_file(FLAGS_file);
  if (!data)
  {
    return EXIT_FAILURE;
  }

  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Uploading template from " << FLAGS_file
            << " to buffer " << static_cast<int>(buffer_id) << "..." << std::endl;
  std::cout << "Template size: " << data->size() << " bytes" << std::endl;

  auto result = sensor.upload_model(*data, buffer_id);
  print_result(result, "Upload model");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_erase(fingerprint_sensor& sensor)
{
  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint16_t delete_count = (FLAGS_count == 0) ? 1 : static_cast<std::uint16_t>(FLAGS_count);

  std::cout << "Erasing " << delete_count << " template(s) starting from position "
            << id << "..." << std::endl;

  auto result = sensor.erase_model(id, delete_count);
  print_result(result, "Delete template(s)");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_erase_all(fingerprint_sensor& sensor)
{
  std::cout << "Erasing all templates..." << std::endl;
  std::cout << "WARNING: This will delete ALL fingerprints!" << std::endl;

  auto result = sensor.clear_database();
  print_result(result, "Clear database");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_match(fingerprint_sensor& sensor)
{
  std::cout << "Matching buffers 1 and 2..." << std::endl;

  auto result = sensor.match_model();
  if (result)
  {
    std::cout << "SUCCESS: Match score = " << result.value().confidence << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Match model");
    return EXIT_FAILURE;
  }
}

int cmd_search(fingerprint_sensor& sensor)
{
  std::uint16_t page_id = static_cast<std::uint16_t>(FLAGS_page);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);
  std::uint16_t count = static_cast<std::uint16_t>(FLAGS_count);

  std::cout << "Searching database (buffer " << static_cast<int>(buffer_id)
            << ", start=" << page_id << ", count=" << count << ")..." << std::endl;

  auto result = sensor.search_model(page_id, buffer_id, count);
  if (result)
  {
    std::cout << "SUCCESS: Found at position " << result.value().index
              << " with score " << result.value().confidence << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Search");
    return EXIT_FAILURE;
  }
}

int cmd_fast_search(fingerprint_sensor& sensor)
{
  std::uint16_t page_id = static_cast<std::uint16_t>(FLAGS_page);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);
  std::uint16_t count = static_cast<std::uint16_t>(FLAGS_count);

  std::cout << "Fast searching database (buffer " << static_cast<int>(buffer_id)
            << ", start=" << page_id << ", count=" << count << ")..." << std::endl;

  auto result = sensor.fast_search_model(page_id, buffer_id, count);
  if (result)
  {
    std::cout << "SUCCESS: Found at position " << result.value().index
              << " with score " << result.value().confidence << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Fast search");
    return EXIT_FAILURE;
  }
}

int cmd_count(fingerprint_sensor& sensor)
{
  std::cout << "Querying template count..." << std::endl;

  auto result = sensor.model_count();
  if (result)
  {
    std::cout << "SUCCESS: Database contains " << result.value() << " template(s)" << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Get count");
    return EXIT_FAILURE;
  }
}

int cmd_led(fingerprint_sensor& sensor)
{
  if (FLAGS_led == 0)
  {
    std::cout << "Turning LED off..." << std::endl;
    auto result = sensor.turn_led_off();
    print_result(result, "LED off");
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else
  {
    std::cout << "Turning LED on..." << std::endl;
    auto result = sensor.turn_led_on();
    print_result(result, "LED on");
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
  }
}

int cmd_enroll(fingerprint_sensor& sensor)
{
  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint16_t samples = static_cast<std::uint16_t>(FLAGS_samples);

  std::cout << "Enrolling fingerprint at position " << id << std::endl;
  std::cout << "Samples required: " << samples << std::endl;
  std::cout << std::endl;

  // Use progress callback for user feedback
  auto progress_callback = [](int step, int total, const char* message) {
    std::cout << "[" << step << "/" << total << "] " << message << std::endl;
  };

  auto result = sensor.enroll_with_progress(id, progress_callback, samples);
  print_result(result, "Enroll");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cmd_verify(fingerprint_sensor& sensor)
{
  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::chrono::milliseconds timeout(FLAGS_timeout);

  std::cout << "Verifying fingerprint against position " << id << std::endl;
  std::cout << "Place finger on sensor..." << std::endl;
  std::cout << std::endl;

  auto progress_callback = [](int step, const char* message) {
    std::cout << "[Step " << step << "] " << message << std::endl;
  };

  auto result = sensor.verify_with_progress(id, progress_callback, timeout);
  if (result)
  {
    std::cout << "SUCCESS: Verified with score " << result.value().confidence << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Verify");
    return EXIT_FAILURE;
  }
}

int cmd_identify(fingerprint_sensor& sensor)
{
  std::chrono::milliseconds timeout(FLAGS_timeout);

  std::cout << "Identifying fingerprint in database..." << std::endl;
  std::cout << "Place finger on sensor..." << std::endl;
  std::cout << std::endl;

  auto progress_callback = [](int step, const char* message) {
    std::cout << "[Step " << step << "] " << message << std::endl;
  };

  auto result = sensor.identify_with_progress(progress_callback, timeout);
  if (result)
  {
    std::cout << "SUCCESS: Identified as position " << result.value().index
              << " with score " << result.value().confidence << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    print_result(result, "Identify");
    return EXIT_FAILURE;
  }
}

int cmd_export(fingerprint_sensor& sensor)
{
  if (FLAGS_file.empty())
  {
    std::cerr << "Error: --file required for export" << std::endl;
    return EXIT_FAILURE;
  }

  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Exporting template from flash position " << id << " to file..." << std::endl;

  // Step 1: Load template from flash to buffer
  std::cout << "Loading template from position " << id << " to buffer "
            << static_cast<int>(buffer_id) << "..." << std::endl;
  auto load_result = sensor.load_model(id, buffer_id);
  if (!load_result)
  {
    print_result(load_result, "Load template from flash");
    return EXIT_FAILURE;
  }

  // Step 2: Download template from buffer
  std::cout << "Downloading template from buffer " << static_cast<int>(buffer_id) << "..." << std::endl;
  auto download_result = sensor.download_model(buffer_id);
  if (!download_result)
  {
    print_result(download_result, "Download template");
    return EXIT_FAILURE;
  }

  std::cout << "Template size: " << download_result.value().size() << " bytes" << std::endl;

  // Step 3: Write to file
  if (write_file(FLAGS_file, download_result.value()))
  {
    std::cout << "SUCCESS: Template exported to " << FLAGS_file << std::endl;
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}

int cmd_import(fingerprint_sensor& sensor)
{
  if (FLAGS_file.empty())
  {
    std::cerr << "Error: --file required for import" << std::endl;
    return EXIT_FAILURE;
  }

  std::uint16_t id = static_cast<std::uint16_t>(FLAGS_id);
  std::uint8_t buffer_id = static_cast<std::uint8_t>(FLAGS_buffer);

  std::cout << "Importing template from file " << FLAGS_file << " to flash position "
            << id << "..." << std::endl;

  // Step 1: Read file
  auto data = read_file(FLAGS_file);
  if (!data)
  {
    return EXIT_FAILURE;
  }

  std::cout << "Template size: " << data->size() << " bytes" << std::endl;

  // Step 2: Upload to buffer
  std::cout << "Uploading template to buffer " << static_cast<int>(buffer_id) << "..." << std::endl;
  auto upload_result = sensor.upload_model(*data, buffer_id);
  if (!upload_result)
  {
    print_result(upload_result, "Upload template");
    return EXIT_FAILURE;
  }

  // Step 3: Store to flash
  std::cout << "Storing template to flash position " << id << "..." << std::endl;
  auto store_result = sensor.store_model(id, buffer_id);
  if (!store_result)
  {
    print_result(store_result, "Store template to flash");
    return EXIT_FAILURE;
  }

  std::cout << "SUCCESS: Template imported from " << FLAGS_file
            << " and stored at position " << id << std::endl;
  return EXIT_SUCCESS;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
  gflags::SetUsageMessage(R"(
carbiok - carbio fingerprint sensor CLI

USAGE:
  carbiok [OPTIONS] <COMMAND>

EXAMPLES:
  # Capture image
  carbiok --port /dev/ttyAMA0 --capture

  # Extract features to buffer 1
  carbiok --extract --buffer 1

  # Enroll fingerprint at position 5
  carbiok --enroll --id 5 --samples 12

  # Verify against ID 5
  carbiok --verify --id 5

  # Search database
  carbiok --identify

  # Download template from buffer 1 to file
  carbiok --down --buffer 1 --file template.bin

  # Upload template from file to buffer 1
  carbiok --up --buffer 1 --file template.bin

  # Store buffer 1 to position 10
  carbiok --store --id 10 --buffer 1

  # Load from position 10 to buffer 2
  carbiok --load --id 10 --buffer 2

  # Match buffers 1 and 2
  carbiok --match

  # Fast search from buffer 1
  carbiok --fast_search --buffer 1 --page 0 --count 200

  # Get template count
  carbiok --get_count

  # Delete template at position 5
  carbiok --erase --id 5

  # Delete 3 templates starting from position 5
  carbiok --erase-all --id 5 --count 3

  # Clear all templates
  carbiok --erase-all

  # Export template from flash position 5 to file
  carbiok --export --id 5 --file template.bin

  # Import template from file to flash position 10
  carbiok --import --id 10 --file template.bin

  # Turn LED on
  carbiok --led 1

  # Turn LED off
  carbiok --led 0
)");

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Count number of commands
  int command_count = 0;
  command_count += FLAGS_capture ? 1 : 0;
  command_count += FLAGS_extract ? 1 : 0;
  command_count += FLAGS_merge ? 1 : 0;
  command_count += FLAGS_store ? 1 : 0;
  command_count += FLAGS_load ? 1 : 0;
  command_count += FLAGS_down ? 1 : 0;
  command_count += FLAGS_up ? 1 : 0;
  command_count += FLAGS_erase ? 1 : 0;
  command_count += FLAGS_erase_all ? 1 : 0;
  command_count += FLAGS_match ? 1 : 0;
  command_count += FLAGS_search ? 1 : 0;
  command_count += FLAGS_fast_search ? 1 : 0;
  command_count += FLAGS_get_count ? 1 : 0;
  command_count += FLAGS_export ? 1 : 0;
  command_count += FLAGS_import ? 1 : 0;
  command_count += (FLAGS_led >= 0) ? 1 : 0;
  command_count += FLAGS_enroll ? 1 : 0;
  command_count += FLAGS_verify ? 1 : 0;
  command_count += FLAGS_identify ? 1 : 0;

  if (command_count == 0)
  {
    std::cerr << "Error: No command specified" << std::endl;
    gflags::ShowUsageWithFlags(argv[0]);
    return EXIT_FAILURE;
  }

  if (command_count > 1)
  {
    std::cerr << "Error: Multiple commands specified, only one allowed" << std::endl;
    return EXIT_FAILURE;
  }

  // Connect to sensor
  std::cout << "Connecting to sensor on " << FLAGS_port << "..." << std::endl;

  fingerprint_sensor sensor;
  if (!sensor.connect(FLAGS_port.c_str()))
  {
    std::cerr << "Error: Failed to connect to sensor" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Connected successfully" << std::endl;
  std::cout << std::endl;

  // Execute command
  int result = EXIT_FAILURE;

  if (FLAGS_capture)
    result = cmd_capture(sensor);
  else if (FLAGS_extract)
    result = cmd_extract(sensor);
  else if (FLAGS_merge)
    result = cmd_merge(sensor);
  else if (FLAGS_store)
    result = cmd_store(sensor);
  else if (FLAGS_load)
    result = cmd_load(sensor);
  else if (FLAGS_down)
    result = cmd_download(sensor);
  else if (FLAGS_up)
    result = cmd_upload(sensor);
  else if (FLAGS_erase)
    result = cmd_erase(sensor);
  else if (FLAGS_erase_all)
    result = cmd_erase_all(sensor);
  else if (FLAGS_match)
    result = cmd_match(sensor);
  else if (FLAGS_search)
    result = cmd_search(sensor);
  else if (FLAGS_fast_search)
    result = cmd_fast_search(sensor);
  else if (FLAGS_get_count)
    result = cmd_count(sensor);
  else if (FLAGS_export)
    result = cmd_export(sensor);
  else if (FLAGS_import)
    result = cmd_import(sensor);
  else if (FLAGS_led >= 0)
    result = cmd_led(sensor);
  else if (FLAGS_enroll)
    result = cmd_enroll(sensor);
  else if (FLAGS_verify)
    result = cmd_verify(sensor);
  else if (FLAGS_identify)
    result = cmd_identify(sensor);

  // Disconnect
  sensor.disconnect();

  return result;
}
