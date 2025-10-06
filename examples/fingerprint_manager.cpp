#include "carbio/fingerprint.h"
#include "print_helper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

#include <algorithm>
#include <iostream>
#include <limits>
#include <thread>

using namespace carbio;

QTextStream cin(stdin);
QTextStream cout(stdout);

bool getFingerprint(FingerprintSensor &sensor)
{
  cout << "Waiting for image..." << Qt::endl;
  cout.flush();

  while (sensor.captureImage() != StatusCode::Success)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  cout << "Templating..." << Qt::endl;
  cout.flush();

  if (sensor.imageToTemplate(1) != StatusCode::Success)
  {
    return false;
  }

  cout << "Searching..." << Qt::endl;
  cout.flush();

  auto result = sensor.fastSearch();

  return result.has_value();
}

bool getFingerprintDetail(FingerprintSensor &sensor)
{
  cout << "Getting image...";
  cout.flush();

  StatusCode status;
  while ((status = sensor.captureImage()) == StatusCode::NoFinger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (status == StatusCode::Success)
  {
    cout << "Image taken" << Qt::endl;
  }
  else
  {
    if (status == StatusCode::NoFinger)
      cout << "No finger detected" << Qt::endl;
    else if (status == StatusCode::ImageCaptureFailed)
      cout << "Imaging error" << Qt::endl;
    else
      cout << "Other error" << Qt::endl;
    return false;
  }

  cout << "Templating...";
  cout.flush();

  status = sensor.imageToTemplate(1);
  if (status == StatusCode::Success)
  {
    cout << "Templated" << Qt::endl;
  }
  else
  {
    if (status == StatusCode::ImageQualityPoor)
      cout << "Image quality too poor" << Qt::endl;
    else if (status == StatusCode::FeatureExtractionFailed)
      cout << "Could not identify features" << Qt::endl;
    else if (status == StatusCode::InvalidImage)
      cout << "Image invalid" << Qt::endl;
    else
      cout << "Other error" << Qt::endl;
    return false;
  }

  cout << "Searching...";
  cout.flush();

  auto result = sensor.fastSearch();
  if (result)
  {
    cout << "Found fingerprint!" << Qt::endl;
    return true;
  }
  else
  {
    cout << "No match found" << Qt::endl;
    return false;
  }
}

/// @brief Enroll a new fingerprint
bool enrollFinger(FingerprintSensor &sensor, uint16_t location)
{
  for (int fingerImg = 1; fingerImg <= 2; ++fingerImg)
  {
    if (fingerImg == 1)
      cout << "Place finger on sensor...";
    else
      cout << "Place same finger again...";
    cout.flush();

    while (true)
    {
      auto status = sensor.captureImage();
      if (status == StatusCode::Success)
      {
        cout << "Image taken" << Qt::endl;
        break;
      }
      if (status == StatusCode::NoFinger)
      {
        cout << ".";
        cout.flush();
      }
      else if (status == StatusCode::ImageCaptureFailed)
      {
        cout << "Imaging error" << Qt::endl;
        return false;
      }
      else
      {
        cout << "Other error" << Qt::endl;
        return false;
      }
    }

    cout << "Templating...";
    cout.flush();

    auto status = sensor.imageToTemplate(fingerImg);
    if (status == StatusCode::Success)
    {
      cout << "Templated" << Qt::endl;
    }
    else
    {
      if (status == StatusCode::ImageQualityPoor)
        cout << "Image quality too poor" << Qt::endl;
      else if (status == StatusCode::FeatureExtractionFailed)
        cout << "Could not identify features" << Qt::endl;
      else if (status == StatusCode::InvalidImage)
        cout << "Image invalid" << Qt::endl;
      else
        cout << "Other error" << Qt::endl;
      return false;
    }

    if (fingerImg == 1)
    {
      cout << "Remove finger" << Qt::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));

      while (sensor.captureImage() != StatusCode::NoFinger)
      {
        // Wait for finger to be removed
      }
    }
  }

  cout << "Creating model...";
  cout.flush();

  auto status = sensor.createModel();
  if (status == StatusCode::Success)
  {
    cout << "Created" << Qt::endl;
  }
  else
  {
    if (status == StatusCode::EnrollMismatch)
      cout << "Prints did not match" << Qt::endl;
    else
      cout << "Other error" << Qt::endl;
    return false;
  }

  cout << "Storing model #" << location << "...";
  cout.flush();

  status = sensor.storeTemplate(location);
  if (status == StatusCode::Success)
  {
    cout << "Stored" << Qt::endl;
  }
  else
  {
    if (status == StatusCode::InvalidLocation)
      cout << "Bad storage location" << Qt::endl;
    else if (status == StatusCode::FlashError)
      cout << "Flash storage error" << Qt::endl;
    else
      cout << "Other error" << Qt::endl;
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
    cout << "Enter ID # from 1-127: ";
    cout.flush();

    QString line = cin.readLine();
    bool    ok;
    id = line.toInt(&ok);

    if (!ok || id < 1 || id > 127)
    {
      id = 0;
    }
  }
  return static_cast<uint16_t>(id);
}

/// @brief Identify fingerprint (find without knowing ID)
bool identifyFingerprint(FingerprintSensor &sensor)
{
  cout << "Place finger on sensor..." << Qt::endl;
  cout.flush();

  StatusCode status;
  while ((status = sensor.captureImage()) == StatusCode::NoFinger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (status != StatusCode::Success)
  {
    cout << "Failed to capture image" << Qt::endl;
    return false;
  }

  cout << "Image captured, templating..." << Qt::endl;

  if (sensor.imageToTemplate(1) != StatusCode::Success)
  {
    cout << "Failed to template image" << Qt::endl;
    return false;
  }

  cout << "Searching database..." << Qt::endl;

  auto result = sensor.fastSearch();
  if (result)
  {
    cout << "Match found! ID: " << result->fingerId << ", Confidence: " << result->confidence << Qt::endl;
    return true;
  }
  else
  {
    cout << "No match found in database" << Qt::endl;
    return false;
  }
}

/// @brief Verify specific fingerprint by ID
bool verifyFingerprint(FingerprintSensor &sensor, uint16_t expectedId)
{
  cout << "Place finger on sensor..." << Qt::endl;
  cout.flush();

  StatusCode status;
  while ((status = sensor.captureImage()) == StatusCode::NoFinger)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (status != StatusCode::Success)
  {
    cout << "Failed to capture image" << Qt::endl;
    return false;
  }

  cout << "Image captured, templating..." << Qt::endl;

  if (sensor.imageToTemplate(1) != StatusCode::Success)
  {
    cout << "Failed to template image" << Qt::endl;
    return false;
  }

  cout << "Loading template #" << expectedId << " for comparison..." << Qt::endl;

  if (sensor.loadTemplate(expectedId, 2) != StatusCode::Success)
  {
    cout << "Failed to load template #" << expectedId << Qt::endl;
    return false;
  }

  cout << "Comparing fingerprints..." << Qt::endl;

  // Search in a small range around the expected ID
  auto result = sensor.search(1, expectedId, 1);
  if (result && result->fingerId == expectedId)
  {
    cout << "Verification SUCCESS! Confidence: " << result->confidence << Qt::endl;
    return true;
  }
  else
  {
    cout << "Verification FAILED - fingerprint does not match ID #" << expectedId << Qt::endl;
    return false;
  }
}

/// @brief Query if a specific template ID exists
void queryTemplate(FingerprintSensor &sensor, uint16_t templateId)
{
  auto templates = sensor.fetchTemplates();
  if (!templates)
  {
    cout << "Failed to fetch templates" << Qt::endl;
    return;
  }

  bool found = std::find(templates->begin(), templates->end(), templateId) != templates->end();
  if (found)
  {
    cout << "Template #" << templateId << " EXISTS in database" << Qt::endl;

    // Try to load it to verify it's valid
    if (sensor.loadTemplate(templateId, 1) == StatusCode::Success)
    {
      cout << "Template is valid and loadable" << Qt::endl;
    }
    else
    {
      cout << "Warning: Template exists in index but failed to load" << Qt::endl;
    }
  }
  else
  {
    cout << "Template #" << templateId << " does NOT exist in database" << Qt::endl;
  }
}

/// @brief LED control menu
void ledControl(FingerprintSensor &sensor)
{
  cout << "\nLED Control:" << Qt::endl;
  cout << "1) Turn LED ON" << Qt::endl;
  cout << "2) Turn LED OFF" << Qt::endl;
  cout << "3) Toggle LED" << Qt::endl;

  cout << "Select option (1-3): ";
  cout.flush();
  QString choice = cin.readLine().trimmed();

  if (choice == "1")
  {
    if (sensor.turnLedOn() == StatusCode::Success)
      cout << "LED turned ON" << Qt::endl;
    else
      cout << "Failed to turn LED on" << Qt::endl;
  }
  else if (choice == "2")
  {
    if (sensor.turnLedOff() == StatusCode::Success)
      cout << "LED turned OFF" << Qt::endl;
    else
      cout << "Failed to turn LED off" << Qt::endl;
  }
  else if (choice == "3")
  {
    cout << "Current state? (on/off): ";
    cout.flush();
    QString state = cin.readLine().trimmed().toLower();

    if (state == "on")
    {
      if (sensor.turnLedOff() == StatusCode::Success)
        cout << "LED turned OFF" << Qt::endl;
      else
        cout << "Failed to turn LED off" << Qt::endl;
    }
    else
    {
      if (sensor.turnLedOn() == StatusCode::Success)
        cout << "LED turned ON" << Qt::endl;
      else
        cout << "Failed to turn LED on" << Qt::endl;
    }
  }
}

/// @brief System configuration menu
void configureSystem(FingerprintSensor &sensor)
{
  cout << "\nSystem Configuration:" << Qt::endl;
  cout << "1) Set baud rate" << Qt::endl;
  cout << "2) Set security level" << Qt::endl;
  cout << "3) Set data packet size" << Qt::endl;
  cout << "4) Set password" << Qt::endl;
  cout << "5) Show current settings" << Qt::endl;

  cout << "Select option (1-5): ";
  cout.flush();
  QString choice = cin.readLine().trimmed();

  if (choice == "1")
  {
    cout << "\nBaud Rates:" << Qt::endl;
    cout << "(1) 9600   (2) 19200  (3) 28800  (4) 38400   (5) 48000   6) 57600" << Qt::endl;
    cout << "(7) 67200  (8) 76800  (9) 86400 (10) 96000  (11) 105600  (12) 115200" << Qt::endl;
    cout << "Select baud rate: ";
    cout.flush();

    QString line = cin.readLine();
    bool    ok;
    int     baudChoice = line.toInt(&ok);

    if (ok && baudChoice >= 1)
    {
      BaudRateRegister baud;
      switch (baudChoice)
      {
        case 1:
          baud = BaudRateRegister::_9600;
          break;
        case 2:
          baud = BaudRateRegister::_19200;
          break;
        case 3:
          baud = BaudRateRegister::_28800;
          break;
        case 4:
          baud = BaudRateRegister::_38400;
          break;
        case 5:
          baud = BaudRateRegister::_48000;
          break;
        case 6:
          baud = BaudRateRegister::_57600;
          break;
        case 7:
          baud = BaudRateRegister::_67200;
          break;
        case 8:
          baud = BaudRateRegister::_76800;
          break;
        case 9:
          baud = BaudRateRegister::_86400;
          break;
        case 10:
          baud = BaudRateRegister::_96000;
          break;
        case 11:
          baud = BaudRateRegister::_105600;
          break;
        case 12:
          baud = BaudRateRegister::_115200;
          break;
        default:
          cout << "Invalid baud rate" << Qt::endl;
          return;
      }

      if (sensor.setBaudRate(baud) == StatusCode::Success)
        cout << "Baud rate updated successfully. Reconnect required." << Qt::endl;
      else
        cout << "Failed to set baud rate" << Qt::endl;
    }
  }
  else if (choice == "2")
  {
    cout << "\nSecurity Levels:" << Qt::endl;
    cout << "1) Lowest  2) Low  3) Balanced  4) High  5) Highest" << Qt::endl;
    cout << "Select security level (1-5): ";
    cout.flush();

    QString line = cin.readLine();
    bool    ok;
    int     level = line.toInt(&ok);

    if (ok && level >= 1 && level <= 5)
    {
      SecurityLevelRegister secLevel = static_cast<SecurityLevelRegister>(level);
      if (sensor.setSecurityLevelRegister(secLevel) == StatusCode::Success)
        cout << "Security level updated successfully" << Qt::endl;
      else
        cout << "Failed to set security level" << Qt::endl;
    }
    else
    {
      cout << "Invalid input" << Qt::endl;
    }
  }
  else if (choice == "3")
  {
    cout << "\nData Packet Sizes:" << Qt::endl;
    cout << "0) 32 bytes  1) 64 bytes  2) 128 bytes  3) 256 bytes" << Qt::endl;
    cout << "Select packet size (0-3): ";
    cout.flush();

    QString line = cin.readLine();
    bool    ok;
    int     size = line.toInt(&ok);

    if (ok && size >= 0 && size <= 3)
    {
      DataPacketSizeRegister packetSize = static_cast<DataPacketSizeRegister>(size);
      if (sensor.setDataPacketSizeRegister(packetSize) == StatusCode::Success)
        cout << "Data packet size updated successfully" << Qt::endl;
      else
        cout << "Failed to set data packet size" << Qt::endl;
    }
    else
    {
      cout << "Invalid input" << Qt::endl;
    }
  }
  else if (choice == "4")
  {
    cout << "Password change not implemented in module" << Qt::endl;
  }
  else if (choice == "5")
  {
    auto params = sensor.getParameters();
    if (params)
    {
      cout << "\nCurrent System Settings:" << Qt::endl;
      cout << "Status Register: 0x" << Qt::hex << params->statusRegister << Qt::endl;
      cout << "System ID: 0x" << Qt::hex << params->systemId << Qt::endl;
      cout << "Library Size: " << Qt::dec << params->capacity << Qt::endl;
      cout << "Security Level: " << params->SecurityLevel << Qt::endl;
      cout << "Device Address: 0x" << Qt::hex << params->deviceAddress << Qt::endl;
      cout << "Data Packet Size: " << Qt::dec << params->packetLength << Qt::endl;
      cout << "Baud Rate: " << params->baudRate << Qt::endl;
    }
    else
    {
      cout << "Failed to read system parameters" << Qt::endl;
    }
  }
}

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);

  FingerprintSensor sensor("/dev/ttyAMA0");
  if (!sensor.begin())  // Auto-detect baud rate
  {
    qCritical() << "Failed to connect to fingerprint sensor";
    return 1;
  }

  // Main menu loop
  while (true)
  {
    cout << "----------------" << Qt::endl;

    // Fetch and display templates
    auto templates = sensor.fetchTemplates();
    if (templates)
    {
      cout << "Fingerprint templates: [";
      for (size_t i = 0; i < templates->size(); ++i)
      {
        if (i > 0)
          cout << ", ";
        cout << (*templates)[i];
      }
      cout << "]" << Qt::endl;
    }
    else
    {
      cout << "Failed to read templates" << Qt::endl;
    }

    cout << "e) enroll print" << Qt::endl;
    cout << "f) find print" << Qt::endl;
    cout << "i) identify print" << Qt::endl;
    cout << "v) verify print" << Qt::endl;
    cout << "q) query print by ID" << Qt::endl;
    cout << "d) delete print" << Qt::endl;
    cout << "c) clear prints" << Qt::endl;
    cout << "l) LED control" << Qt::endl;
    cout << "s) system config" << Qt::endl;
    cout << "r) soft reset sensor" << Qt::endl;
    cout << "x) quit" << Qt::endl;
    cout << "----------------" << Qt::endl;
    cout << "> ";
    cout.flush();

    QString input = cin.readLine().trimmed();

    if (input.toLower() == "e")
    {
      uint16_t id = getTemplateId();
      if (enrollFinger(sensor, id))
      {
        cout << "Enrollment successful!" << Qt::endl;
      }
      else
      {
        cout << "Enrollment failed" << Qt::endl;
      }
    }
    else if (input.toLower() == "f")
    {
      if (getFingerprintDetail(sensor))
      {
        auto result = sensor.getLastSearchResult();
        if (result)
        {
          cout << "Detected #" << result->fingerId << " with confidence " << result->confidence << Qt::endl;
        }
      }
      else
      {
        cout << "Finger not found" << Qt::endl;
      }
    }
    else if (input.toLower() == "i")
    {
      identifyFingerprint(sensor);
    }
    else if (input.toLower() == "v")
    {
      uint16_t id = getTemplateId();
      verifyFingerprint(sensor, id);
    }
    else if (input.toLower() == "q")
    {
      uint16_t id = getTemplateId();
      queryTemplate(sensor, id);
    }
    else if (input.toLower() == "d")
    {
      uint16_t id = getTemplateId();
      if (sensor.deleteTemplate(id, 1) == StatusCode::Success)
      {
        cout << "Deleted!" << Qt::endl;
      }
      else
      {
        cout << "Failed to delete" << Qt::endl;
      }
    }
    else if (input == "c")
    {
      cout << "WARNING: This will clear fingerprints!" << Qt::endl;
      cout << "Type 'y' to confirm: ";
      cout.flush();
      QString confirm = cin.readLine().trimmed();
      if (confirm == "y")
      {
        cout << "Clearing database..." << Qt::endl;
        if (sensor.clearDatabase() == StatusCode::Success)
        {
          cout << "All fingerprints deleted!" << Qt::endl;
        }
        else
        {
          cout << "Failed to clear database" << Qt::endl;
        }
      }
      else
      {
        cout << "Cancelled" << Qt::endl;
      }
    }
    else if (input.toLower() == "l")
    {
      ledControl(sensor);
    }
    else if (input.toLower() == "s")
    {
      configureSystem(sensor);
    }
    else if (input.toLower() == "r")
    {
      cout << "Soft resetting sensor..." << Qt::endl;
      if (sensor.softReset() == StatusCode::Success)
      {
        cout << "Sensor reset successfully" << Qt::endl;
        // Display updated parameters
        auto params = sensor.getParameters();
        if (params)
        {
          cout << "Current settings after reset:" << Qt::endl;
          cout << "  Baud Rate: " << params->baudRate << Qt::endl;
          cout << "  Security Level: " << params->SecurityLevel << Qt::endl;
          cout << "  Packet Length: " << params->packetLength << Qt::endl;
        }
      }
      else
      {
        cout << "Failed to reset sensor" << Qt::endl;
      }
    }
    else if (input.toLower() == "x")
    {
      cout << "Disconnecting..." << Qt::endl;
      sensor.end();
      break;
    }
  }

  return 0;
}
