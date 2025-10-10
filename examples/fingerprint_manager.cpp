#include "carbio/fingerprint_sensor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

using namespace carbio;

// Helper function to get device capacity for searches
uint16_t getDeviceCapacity(fingerprint_sensor &sensor)
{
  auto params = sensor.get_device_setting_info();
  return params ? params->capacity : 127; // Default to 127 if read fails
}

bool getFingerprint(fingerprint_sensor &sensor)
{
  std::cout << "Waiting for image..." << std::endl;

  while (!sensor.capture_image())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  std::cout << "Templating..." << std::endl;

  if (!sensor.extract_features(1))
  {
    return false;
  }

  std::cout << "Searching..." << std::endl;

  uint16_t capacity = getDeviceCapacity(sensor);
  auto result = sensor.fast_search_model(0, 1, capacity);

  return result.has_value();
}

std::optional<search_query_info> getFingerprintDetail(fingerprint_sensor &sensor)
{
  std::cout << "Getting image..." << std::flush;

  void_result status;
  while ((status = sensor.capture_image()).error() == status_code::no_finger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (status)
  {
    std::cout << "Image taken" << std::endl;
  }
  else
  {
    std::cout << "Error: " << get_message(status.error()) << std::endl;
    return std::nullopt;
  }

  std::cout << "Templating..." << std::flush;

  status = sensor.extract_features(1);
  if (status)
  {
    std::cout << "Templated" << std::endl;
  }
  else
  {
    std::cout << "Error: " << get_message(status.error()) << std::endl;
    return std::nullopt;
  }

  std::cout << "Searching..." << std::flush;

  uint16_t capacity = getDeviceCapacity(sensor);
  auto result = sensor.fast_search_model(0, 1, capacity);
  if (result)
  {
    std::cout << "Found fingerprint!" << std::endl;
    return *result; // Convert result<T> to optional<T>
  }
  else
  {
    std::cout << "No match found" << std::endl;
    return std::nullopt;
  }
}

/// @brief Enroll a new fingerprint
bool enrollFinger(fingerprint_sensor &sensor, uint16_t location)
{
  for (int fingerImg = 1; fingerImg <= 2; ++fingerImg)
  {
    if (fingerImg == 1)
      std::cout << "Place finger on sensor..." << std::flush;
    else
      std::cout << "Place same finger again..." << std::flush;

    while (true)
    {
      auto status = sensor.capture_image();
      if (status)
      {
        std::cout << "Image taken" << std::endl;
        break;
      }
      if (status.error() == status_code::no_finger)
      {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
      else
      {
        std::cout << "\nError: " << get_message(status.error()) << std::endl;
        return false;
      }
    }

    std::cout << "Templating..." << std::flush;

    auto status = sensor.extract_features(fingerImg);
    if (status)
    {
      std::cout << "Templated" << std::endl;
    }
    else
    {
      std::cout << "\nError: " << get_message(status.error()) << std::endl;
      return false;
    }

    if (fingerImg == 1)
    {
      std::cout << "Remove finger" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));

      while (sensor.capture_image().error() != status_code::no_finger)
      {
        // Wait for finger to be removed
      }
    }
  }

  std::cout << "Creating model..." << std::flush;

  auto status = sensor.create_model();
  if (status)
  {
    std::cout << "Created" << std::endl;
  }
  else
  {
    std::cout << "\nError: " << get_message(status.error()) << std::endl;
    return false;
  }

  std::cout << "Storing model #" << location << "..." << std::flush;

  status = sensor.store_model(location);
  if (status)
  {
    std::cout << "Stored" << std::endl;
  }
  else
  {
    std::cout << "\nError: " << get_message(status.error()) << std::endl;
    return false;
  }

  return true;
}

/// @brief Get a valid template ID from user (1-127)
uint16_t getTemplateId()
{
  int id = 0;
  while (id < 1 || id > 127)
  {
    std::cout << "Enter ID # from 1-127: " << std::flush;

    std::string line;
    std::getline(std::cin, line);

    try
    {
      id = std::stoi(line);
      if (id < 1 || id > 127)
      {
        id = 0;
      }
    }
    catch (...)
    {
      id = 0;
    }
  }
  return static_cast<uint16_t>(id);
}

/// @brief Identify fingerprint (find without knowing ID)
bool identifyFingerprint(fingerprint_sensor &sensor)
{
  std::cout << "Place finger on sensor..." << std::endl;

  void_result status;
  while ((status = sensor.capture_image()).error() == status_code::no_finger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!status)
  {
    std::cout << "Failed to capture image" << std::endl;
    return false;
  }

  std::cout << "Image captured, templating..." << std::endl;

  if (!sensor.extract_features(1))
  {
    std::cout << "Failed to template image" << std::endl;
    return false;
  }

  std::cout << "Searching database..." << std::endl;

  uint16_t capacity = getDeviceCapacity(sensor);
  auto result = sensor.fast_search_model(0, 1, capacity);
  if (result)
  {
    std::cout << "Match found! ID: " << result->index << ", Confidence: " << result->confidence << std::endl;
    return true;
  }
  else
  {
    std::cout << "No match found in database" << std::endl;
    return false;
  }
}

/// @brief Verify specific fingerprint by ID
bool verifyFingerprint(fingerprint_sensor &sensor, uint16_t expectedId)
{
  std::cout << "Place finger on sensor..." << std::endl;

  void_result status;
  while ((status = sensor.capture_image()).error() == status_code::no_finger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (!status)
  {
    std::cout << "Failed to capture image" << std::endl;
    return false;
  }

  std::cout << "Image captured, templating..." << std::endl;

  if (!sensor.extract_features(1))
  {
    std::cout << "Failed to template image" << std::endl;
    return false;
  }

  std::cout << "Loading template #" << expectedId << " for comparison..." << std::endl;

  if (!sensor.load_model(expectedId, 2))
  {
    std::cout << "Failed to load template #" << expectedId << std::endl;
    return false;
  }

  std::cout << "Comparing fingerprints..." << std::endl;

  // Search in a small range around the expected ID
  auto result = sensor.search_model(expectedId, 1, 1);
  if (result && result->index == expectedId)
  {
    std::cout << "Verification SUCCESS! Confidence: " << result->confidence << std::endl;
    return true;
  }
  else
  {
    std::cout << "Verification FAILED - fingerprint does not match ID #" << expectedId << std::endl;
    return false;
  }
}

/// @brief Query if a specific template ID exists
void queryTemplate(fingerprint_sensor &sensor, uint16_t templateId)
{
  std::array<std::uint8_t, 32> buffer;
  auto result = sensor.read_index_table(buffer);
  if (!result)
  {
    std::cout << "Failed to fetch templates" << std::endl;
    return;
  }

  // Check if the bit for this template ID is set (32 bytes = 256 templates max)
  if (templateId >= 256)
  {
    std::cout << "Template ID out of range (max 255)" << std::endl;
    return;
  }

  // Each byte contains 8 templates, bit 0 = ID 0, bit 1 = ID 1, etc.
  std::uint16_t byte_index = templateId / 8;
  std::uint8_t bit_index = templateId % 8;
  bool found = (result->at(byte_index) & (1 << bit_index)) != 0;

  if (found)
  {
    std::cout << "Template #" << templateId << " EXISTS in database" << std::endl;

    // Try to load it to verify it's valid
    if (sensor.load_model(templateId, 1))
    {
      std::cout << "Template is valid and loadable" << std::endl;
    }
    else
    {
      std::cout << "Warning: Template exists in index but failed to load" << std::endl;
    }
  }
  else
  {
    std::cout << "Template #" << templateId << " does NOT exist in database" << std::endl;
  }
}

/// @brief LED control menu
void ledControl(fingerprint_sensor &sensor)
{
  std::cout << "\nLED Control:" << std::endl;
  std::cout << "1) Turn LED ON" << std::endl;
  std::cout << "2) Turn LED OFF" << std::endl;
  std::cout << "3) Toggle LED" << std::endl;

  std::cout << "Select option (1-3): " << std::flush;
  std::string choice;
  std::getline(std::cin, choice);

  if (choice == "1")
  {
    if (sensor.turn_led_on())
      std::cout << "LED turned ON" << std::endl;
    else
      std::cout << "Failed to turn LED on" << std::endl;
  }
  else if (choice == "2")
  {
    if (sensor.turn_led_off())
      std::cout << "LED turned OFF" << std::endl;
    else
      std::cout << "Failed to turn LED off" << std::endl;
  }
  else if (choice == "3")
  {
    std::cout << "Current state? (on/off): " << std::flush;
    std::string state;
    std::getline(std::cin, state);

    if (state == "on")
    {
      if (sensor.turn_led_off())
        std::cout << "LED turned OFF" << std::endl;
      else
        std::cout << "Failed to turn LED off" << std::endl;
    }
    else
    {
      if (sensor.turn_led_on())
        std::cout << "LED turned ON" << std::endl;
      else
        std::cout << "Failed to turn LED on" << std::endl;
    }
  }
}

/// @brief System configuration menu
void configureSystem(fingerprint_sensor &sensor)
{
  std::cout << "\nSystem Configuration:" << std::endl;
  std::cout << "1) Set baud rate" << std::endl;
  std::cout << "2) Set security level" << std::endl;
  std::cout << "3) Set data packet size" << std::endl;
  std::cout << "4) Set password" << std::endl;
  std::cout << "5) Show current settings" << std::endl;

  std::cout << "Select option (1-5): " << std::flush;
  std::string choice;
  std::getline(std::cin, choice);

  if (choice == "1")
  {
    std::cout << "\nBaud Rates:" << std::endl;
    std::cout << "(1) 9600   (2) 19200  (3) 28800  (4) 38400   (5) 48000   6) 57600" << std::endl;
    std::cout << "(7) 67200  (8) 76800  (9) 86400 (10) 96000  (11) 105600  (12) 115200" << std::endl;
    std::cout << "Select baud rate: " << std::flush;

    std::string line;
    std::getline(std::cin, line);

    try
    {
      int baudChoice = std::stoi(line);
      if (baudChoice >= 1)
    {
      baud_rate_setting baud;
      switch (baudChoice)
      {
        case 1:
          baud = baud_rate_setting::_9600;
          break;
        case 2:
          baud = baud_rate_setting::_19200;
          break;
        case 3:
          baud = baud_rate_setting::_28800;
          break;
        case 4:
          baud = baud_rate_setting::_38400;
          break;
        case 5:
          baud = baud_rate_setting::_48000;
          break;
        case 6:
          baud = baud_rate_setting::_57600;
          break;
        case 7:
          baud = baud_rate_setting::_67200;
          break;
        case 8:
          baud = baud_rate_setting::_76800;
          break;
        case 9:
          baud = baud_rate_setting::_86400;
          break;
        case 10:
          baud = baud_rate_setting::_96000;
          break;
        case 11:
          baud = baud_rate_setting::_105600;
          break;
        case 12:
          baud = baud_rate_setting::_115200;
          break;
        default:
          std::cout << "Invalid baud rate" << std::endl;
          return;
      }

      if (sensor.set_baud_rate_setting(baud))
        std::cout << "Baud rate updated successfully. Reconnect required." << std::endl;
      else
        std::cout << "Failed to set baud rate" << std::endl;
      }
    }
    catch (...)
    {
      std::cout << "Invalid input" << std::endl;
    }
  }
  else if (choice == "2")
  {
    std::cout << "\nSecurity Levels:" << std::endl;
    std::cout << "1) Lowest  2) Low  3) Balanced  4) High  5) Highest" << std::endl;
    std::cout << "Select security level (1-5): " << std::flush;

    std::string line;
    std::getline(std::cin, line);

    try
    {
      int level = std::stoi(line);
      if (level >= 1 && level <= 5)
      {
        security_level_setting secLevel = static_cast<security_level_setting>(level);
        if (sensor.set_security_level_setting(secLevel))
          std::cout << "Security level updated successfully" << std::endl;
        else
          std::cout << "Failed to set security level" << std::endl;
      }
      else
      {
        std::cout << "Invalid input" << std::endl;
      }
    }
    catch (...)
    {
      std::cout << "Invalid input" << std::endl;
    }
  }
  else if (choice == "3")
  {
    std::cout << "\nData Packet Sizes:" << std::endl;
    std::cout << "0) 32 bytes  1) 64 bytes  2) 128 bytes  3) 256 bytes" << std::endl;
    std::cout << "Select packet size (0-3): " << std::flush;

    std::string line;
    std::getline(std::cin, line);

    try
    {
      int size = std::stoi(line);
      if (size >= 0 && size <= 3)
      {
        data_length_setting packetSize = static_cast<data_length_setting>(size);
        if (sensor.set_data_length_setting(packetSize))
          std::cout << "Data packet size updated successfully" << std::endl;
        else
          std::cout << "Failed to set data packet size" << std::endl;
      }
      else
      {
        std::cout << "Invalid input" << std::endl;
      }
    }
    catch (...)
    {
      std::cout << "Invalid input" << std::endl;
    }
  }
  else if (choice == "4")
  {
    std::cout << "Password change not implemented in module" << std::endl;
  }
  else if (choice == "5")
  {
    auto params = sensor.get_device_setting_info();
    if (params)
    {
      std::cout << "\nCurrent System Settings:" << std::endl;
      std::cout << "Status Register: 0x" << std::hex << params->status << std::endl;
      std::cout << "System ID: 0x" << std::hex << params->id << std::endl;
      std::cout << "Library Size: " << std::dec << params->capacity << std::endl;
      std::cout << "Security Level: " << params->security_level << std::endl;
      std::cout << "Device Address: 0x" << std::hex << params->address << std::endl;
      std::cout << "Data Packet Size: " << std::dec << params->length << std::endl;
      std::cout << "Baud Rate: " << params->baudrate << std::endl;
    }
    else
    {
      std::cout << "Failed to read system parameters" << std::endl;
    }
  }
}

int main(int argc, char *argv[])
{
  spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  spdlog::set_level(spdlog::level::warn);

  fingerprint_sensor sensor;
  if (!sensor.open("/dev/ttyAMA0"))
  {
    std::cerr << "Failed to connect to fingerprint sensor" << std::endl;
    return 1;
  }

  // Main menu loop
  while (true)
  {
    std::cout << "----------------" << std::endl;

    // Fetch and display templates
    std::array<std::uint8_t, 32> buffer;
    auto result = sensor.read_index_table(buffer);
    if (result)
    {
      std::cout << "Fingerprint templates: [";
      bool first = true;
      // Iterate through the bitmap and extract template IDs (32 bytes = 256 templates max)
      for (std::uint16_t i = 0; i < 32 * 8; ++i)
      {
        std::uint16_t byte_index = i / 8;
        std::uint8_t bit_index = i % 8;
        if ((result->at(byte_index) & (1 << bit_index)) != 0)
        {
          if (!first) std::cout << ", ";
          std::cout << i;
          first = false;
        }
      }
      std::cout << "]" << std::endl;
    }
    else
    {
      std::cout << "Failed to read templates" << std::endl;
    }

    std::cout << "e) enroll print" << std::endl;
    std::cout << "f) find print" << std::endl;
    std::cout << "i) identify print" << std::endl;
    std::cout << "v) verify print" << std::endl;
    std::cout << "q) query print by ID" << std::endl;
    std::cout << "d) delete print" << std::endl;
    std::cout << "c) clear prints" << std::endl;
    std::cout << "l) LED control" << std::endl;
    std::cout << "s) system config" << std::endl;
    std::cout << "r) soft reset sensor" << std::endl;
    std::cout << "x) quit" << std::endl;
    std::cout << "----------------" << std::endl;
    std::cout << "> " << std::flush;

    std::string input;
    std::getline(std::cin, input);

    if (input == "e" || input == "E")
    {
      uint16_t id = getTemplateId();
      if (enrollFinger(sensor, id))
      {
        std::cout << "Enrollment successful!" << std::endl;
      }
      else
      {
        std::cout << "Enrollment failed" << std::endl;
      }
    }
    else if (input == "f" || input == "F")
    {
      auto result = getFingerprintDetail(sensor);
      if (result)
      {
        std::cout << "Detected #" << result->index << " with confidence " << result->confidence << std::endl;
      }
      else
      {
        std::cout << "Finger not found" << std::endl;
      }
    }
    else if (input == "i" || input == "I")
    {
      identifyFingerprint(sensor);
    }
    else if (input == "v" || input == "V")
    {
      uint16_t id = getTemplateId();
      verifyFingerprint(sensor, id);
    }
    else if (input == "q" || input == "Q")
    {
      uint16_t id = getTemplateId();
      queryTemplate(sensor, id);
    }
    else if (input == "d" || input == "D")
    {
      uint16_t id = getTemplateId();
      if (sensor.erase_model(id, 1))
      {
        std::cout << "Deleted!" << std::endl;
      }
      else
      {
        std::cout << "Failed to delete" << std::endl;
      }
    }
    else if (input == "c" || input == "C")
    {
      std::cout << "WARNING: This will clear fingerprints!" << std::endl;
      std::cout << "Type 'y' to confirm: " << std::flush;
      std::string confirm;
      std::getline(std::cin, confirm);
      if (confirm == "y" || confirm == "Y")
      {
        std::cout << "Clearing database..." << std::endl;
        if (sensor.clear_database())
        {
          std::cout << "All fingerprints deleted!" << std::endl;
        }
        else
        {
          std::cout << "Failed to clear database" << std::endl;
        }
      }
      else
      {
        std::cout << "Cancelled" << std::endl;
      }
    }
    else if (input == "l" || input == "L")
    {
      ledControl(sensor);
    }
    else if (input == "s" || input == "S")
    {
      configureSystem(sensor);
    }
    else if (input == "r" || input == "R")
    {
      std::cout << "Soft resetting sensor..." << std::endl;
      if (sensor.soft_reset_device())
      {
        std::cout << "Sensor reset successfully" << std::endl;
        // Display updated parameters
        auto params = sensor.get_device_setting_info();
        if (params)
        {
          std::cout << "Current settings after reset:" << std::endl;
          std::cout << "  Baud Rate: " << params->baudrate << std::endl;
          std::cout << "  Security Level: " << params->security_level << std::endl;
          std::cout << "  Packet Length: " << params->length << std::endl;
        }
      }
      else
      {
        std::cout << "Failed to reset sensor" << std::endl;
      }
    }
    else if (input == "x" || input == "X")
    {
      std::cout << "Disconnecting..." << std::endl;
      sensor.close();
      break;
    }
  }

  return 0;
}
