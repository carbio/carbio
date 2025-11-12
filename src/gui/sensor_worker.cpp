/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#include "sensor_worker.h"

#include "fingerprint/fingerprint_sensor.h"
#include "fingerprint/status_flag.h"
#include <QTimer>

#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

SensorWorker::SensorWorker(carbio::fingerprint_sensor* sensor, QObject* parent)
    : QObject(parent)
    , m_sensor(sensor)
    , m_authPollTimer(nullptr)
    , m_adminPollTimer(nullptr)
{
  // DO NOT create timers in constructor!
  // Timers have thread affinity - they run on thread where created
  // Must be created after moveToThread() is called
}

SensorWorker::~SensorWorker()
{
  if (m_authPollTimer)
    m_authPollTimer->stop();
  if (m_adminPollTimer)
    m_adminPollTimer->stop();
}

// ============================================================================
// Timer Initialization (MUST run on worker thread!)
// ============================================================================

void SensorWorker::initializeTimers()
{
  // Create timers on worker thread (correct thread affinity)
  m_authPollTimer = new QTimer(this);
  m_adminPollTimer = new QTimer(this);

  // Connect authentication polling timer (direct invocation - no queue)
  connect(m_authPollTimer, &QTimer::timeout, this, &SensorWorker::onAuthenticationPollTick);

  // Connect admin polling timer (direct invocation - no queue)
  connect(m_adminPollTimer, &QTimer::timeout, this, &SensorWorker::executeAdminPoll);
}

// ============================================================================
// Sensor Initialization (runs on worker thread)
// ============================================================================

void SensorWorker::initializeSensor(const char* portPath)
{
  if (!m_sensor) [[unlikely]]
  {
    SPDLOG_ERROR("Sensor pointer is null!");
    emit sensorInitialized(false);
    return;
  }

  // This blocking call is OK - we're on worker thread!
  bool success = m_sensor->connect(portPath);

  if (!success)
  {
    SPDLOG_WARN("Failed to initialize sensor");
  }

  emit sensorInitialized(success);
}

// ============================================================================
// Simple Authentication Polling (Direct polling at specified interval)
// ============================================================================

void SensorWorker::startAuthenticationPolling(int intervalMs)
{
  if (!m_authPollTimer) [[unlikely]]
    return;
  m_authPollTimer->setInterval(intervalMs);
  m_authPollTimer->start();
}

void SensorWorker::stopAuthenticationPolling()
{
  if (!m_authPollTimer) [[unlikely]]
    return;
  m_authPollTimer->stop();
}

void SensorWorker::onAuthenticationPollTick()
{
  if (!m_sensor) [[unlikely]]
  {
    return;
  }

  // Use HAL's efficient identify() with built-in retry logic
  auto result = m_sensor->identify();

  if (!result) [[unlikely]]
  {
    auto error = result.error();

    // No finger or transient errors - continue polling silently
    if (error == carbio::status_code::no_finger || carbio::is_retryable(error)) [[likely]]
    {
      return;
    }

    // No match or fatal errors
    if (error == carbio::status_code::no_match || error == carbio::status_code::not_found) [[likely]]
    {
      emit authenticationFailed();
      return;
    }

    // Serious error
    emit authenticationFailed();
    return;
  }

  // SUCCESS!
  emit scanProgressUpdate(100);
  emit authenticationSuccess(result->index, result->confidence);
}

void SensorWorker::startAdminPolling(int intervalMs)
{
  if (!m_adminPollTimer) [[unlikely]]
    return;
  m_adminPollTimer->setInterval(intervalMs);
  m_adminPollTimer->start();
}

void SensorWorker::stopAdminPolling()
{
  if (!m_adminPollTimer) [[unlikely]]
    return;
  m_adminPollTimer->stop();
}

void SensorWorker::executeAdminPoll()
{
  if (!m_sensor) [[unlikely]]
  {
    emit adminFingerprintFailed("Sensor not available");
    return;
  }

  // Use HAL's efficient identify() with built-in retry logic
  auto result = m_sensor->identify();

  if (!result) [[unlikely]]
  {
    auto error = result.error();

    // No finger or transient errors - continue polling silently
    if (error == carbio::status_code::no_finger || carbio::is_retryable(error)) [[likely]]
    {
      return;
    }

    // No match - stop polling and report failure
    if (error == carbio::status_code::no_match || error == carbio::status_code::not_found) [[likely]]
    {
      if (m_adminPollTimer)
        m_adminPollTimer->stop();
      emit adminFingerprintFailed("Fingerprint not recognized");
      return;
    }

    // Fatal error - stop polling
    if (m_adminPollTimer)
      m_adminPollTimer->stop();
    emit adminFingerprintFailed("Authentication failed");
    return;
  }

  // SUCCESS! Stop polling
  if (m_adminPollTimer)
    m_adminPollTimer->stop();
  emit adminFingerprintSuccess(result->index, result->confidence);
}

// ============================================================================
// Single-shot authentication (for dialog operations)
// ============================================================================

void SensorWorker::performAuthentication()
{
  if (!m_sensor)
  {
    emit authenticationFailed();
    return;
  }

  emit scanProgressUpdate(5);
  emit progressUpdate("Place finger on sensor...");

  // Use HAL's efficient identify() with built-in retry logic
  auto result = m_sensor->identify();

  if (!result)
  {
    auto error = result.error();

    // No finger or transient errors - signal for retry
    if (error == carbio::status_code::no_finger || carbio::is_retryable(error))
    {
      emit authenticationNoFinger();
      return;
    }

    // No match or fatal errors
    emit authenticationFailed();
    return;
  }

  // SUCCESS!
  emit scanProgressUpdate(100);
  emit authenticationSuccess(result->index, result->confidence);
}

void SensorWorker::performAdminAuthentication()
{
  if (!m_sensor)
  {
    emit adminFingerprintFailed("Sensor not available");
    return;
  }

  // Use HAL's efficient identify() (no progress updates for admin)
  auto result = m_sensor->identify();

  if (!result)
  {
    auto error = result.error();

    // No finger or transient errors - signal for retry
    if (error == carbio::status_code::no_finger || carbio::is_retryable(error))
    {
      emit adminFingerprintNoFinger();
      return;
    }

    // No match or fatal errors
    emit adminFingerprintFailed("Fingerprint not recognized");
    return;
  }

  // SUCCESS!
  emit adminFingerprintSuccess(result->index, result->confidence);
}

void SensorWorker::enrollFingerprint(int id)
{
  SPDLOG_INFO("enrollFingerprint called for BASE ID: {}", id);

  if (!m_sensor)
  {
    SPDLOG_WARN("Sensor not available");
    emit enrollmentFailed("Sensor not available");
    return;
  }

  static constexpr int TEMPLATE_PER_USER = 3;
  static constexpr int SAMPLES_PER_TEMPLATE = 3;

  // Validate ID range (0-127 for R307 sensor)
  if (id < 0 || id > 127)
  {
    SPDLOG_WARN("Invalid ID: {}", id);
    emit enrollmentFailed("Invalid ID. Must be between 0 and 127.");
    return;
  }

  SPDLOG_INFO("Starting enrollment for ID: {} using HAL's enroll_with_progress() with default 12 samples", id);

  // Turn on LED
  m_sensor->turn_led_on();

  emit scanProgressUpdate(0);
  emit enrollmentProgress("Place finger on sensor...");

  // Use HAL's enroll_with_progress() with callback for real-time updates
  auto result = m_sensor->enroll_with_progress(static_cast<uint16_t>(id),
                                               [this](int current_sample, int total_samples, const char* message)
                                               {
                                                 int percentage = (current_sample * 100) / total_samples;
                                                 emit scanProgressUpdate(percentage);
                                                 emit enrollmentProgress(QString::fromUtf8(message) + QString(" (%1/%2)").arg(current_sample).arg(total_samples));
                                               });

  if (result)
  {
    emit scanProgressUpdate(100);
    emit enrollmentComplete("Fingerprint enrolled successfully as ID #" + QString::number(id));
    SPDLOG_INFO("Enrollment successful for ID: {}", id);
  }
  else
  {
    emit scanProgressUpdate(0);
    QString errorMsg = QString::fromStdString(carbio::message(result.error()));
    emit enrollmentFailed("Enrollment failed: " + errorMsg);
    SPDLOG_WARN("Enrollment failed for ID: {} - {}", id, errorMsg.toStdString());
    m_sensor->turn_led_off();
  }
}

void SensorWorker::identifyFingerprint()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  emit scanProgressUpdate(0);
  emit progressUpdate("Place finger on sensor...");

  // Use HAL's identify_with_progress() with callback for real-time updates
  auto result = m_sensor->identify_with_progress(
      [this](int progress_percent, const char* message)
      {
        emit scanProgressUpdate(progress_percent);
        emit progressUpdate(QString::fromUtf8(message));
      });

  if (result)
  {
    emit scanProgressUpdate(100);
    emit operationComplete(QString("ID #%1 (confidence: %2)").arg(result->index).arg(result->confidence));
  }
  else
  {
    emit scanProgressUpdate(0);
    emit operationFailed("No matching fingerprint found");
  }
}

void SensorWorker::verifyFingerprint(int id)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  SPDLOG_INFO("verifyFingerprint - using HAL's verify_with_progress()");

  emit scanProgressUpdate(0);
  emit progressUpdate("Place finger on sensor...");

  // Use HAL's verify_with_progress() with callback for real-time updates
  auto result = m_sensor->verify_with_progress(static_cast<uint16_t>(id),
                                               [this](int progress_percent, const char* message)
                                               {
                                                 emit scanProgressUpdate(progress_percent);
                                                 emit progressUpdate(QString::fromUtf8(message));
                                               });

  if (result)
  {
    emit scanProgressUpdate(100);
    emit operationComplete("VERIFIED - ID #" + QString::number(id) + " (confidence: " + QString::number(result->confidence) + ")");
  }
  else
  {
    emit scanProgressUpdate(0);
    emit operationFailed("Fingerprint does NOT match ID #" + QString::number(id));
  }
}

void SensorWorker::queryTemplate(int id)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  std::array<std::uint8_t, 32> buffer;
  auto result = m_sensor->read_index_table(buffer);
  if (id >= 256)
  {
    emit operationFailed("Template ID out of range (max 255)");
    return;
  }
  std::uint16_t byte_index = id / 8;
  std::uint8_t bit_index = id % 8;
  bool found = (result->at(byte_index) & (1 << bit_index)) != 0;
  if (found && m_sensor->load_model(id, 1))
  {
    emit operationComplete("Template #" + QString::number(id) + " EXISTS in database");
  }
  else
  {
    emit operationComplete("Template #" + QString::number(id) + " does NOT exist");
  }
}

void SensorWorker::deleteFingerprint(int id)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (id < 1 || id > 127)
  {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  if (!m_sensor->erase_model(static_cast<uint16_t>(id), 1))
  {
    emit operationComplete("Fingerprint #" + QString::number(id) + " deleted");
  }
  else
  {
    emit operationFailed("Failed to delete fingerprint #" + QString::number(id));
  }
}

void SensorWorker::clearDatabase()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (!m_sensor->clear_database())
  {
    emit operationComplete("All fingerprints deleted");
  }
  else
  {
    emit operationFailed("Failed to clear database");
  }
}

void SensorWorker::refreshTemplateCount()
{
  if (!m_sensor)
  {
    emit templateCountUpdated(0);
    return;
  }

  auto result = m_sensor->model_count();
  if (result)
  {
    emit templateCountUpdated(static_cast<int>(result.value()));
  }
  else
  {
    emit templateCountUpdated(0);
  }
}

void SensorWorker::getEnrolledTemplateIds()
{
  QVector<int> enrolledIds;

  if (!m_sensor)
  {
    emit enrolledTemplateIds(enrolledIds);
    return;
  }

  // Read the index table (bitmap of which IDs are enrolled)
  std::array<std::uint8_t, 32> buffer;
  auto result = m_sensor->read_index_table(buffer);

  if (!result)
  {
    SPDLOG_WARN("Failed to read sensor index table");
    emit enrolledTemplateIds(enrolledIds);
    return;
  }

  // Parse the bitmap to extract enrolled IDs
  // Each byte contains 8 bits representing 8 template IDs
  for (int byte_index = 0; byte_index < 32; ++byte_index)
  {
    std::uint8_t byte_value = result->at(static_cast<size_t>(byte_index));
    for (int bit_index = 0; bit_index < 8; ++bit_index)
    {
      if ((byte_value & (1 << bit_index)) != 0)
      {
        int template_id = byte_index * 8 + bit_index;
        if (template_id <= 127)
        { // Valid range for Adafruit ID751
          enrolledIds.append(template_id);
        }
      }
    }
  }

  SPDLOG_INFO("Found {} templates on sensor", enrolledIds.size());
  emit enrolledTemplateIds(enrolledIds);
}

void SensorWorker::writeNotepad(int pageNumber, QByteArray data)
{
  if (!m_sensor)
  {
    emit notepadWriteFailed(pageNumber, "Sensor not available");
    return;
  }

  if (pageNumber < 0 || pageNumber > 15)
  {
    emit notepadWriteFailed(pageNumber, "Invalid page number (must be 0-15)");
    return;
  }

  if (data.size() != 32)
  {
    emit notepadWriteFailed(pageNumber, QString("Invalid data size: %1 bytes (must be 32)").arg(data.size()));
    return;
  }

  // Convert QByteArray to std::array<uint8_t, 32>
  std::array<std::uint8_t, 32> buffer;
  std::copy_n(reinterpret_cast<const std::uint8_t*>(data.constData()), 32, buffer.begin());

  // Create span from the array
  std::span<const std::uint8_t, 32> span(buffer);

  auto result = m_sensor->write_notepad(static_cast<std::uint8_t>(pageNumber), span);

  if (result)
  {
    emit notepadWriteComplete(pageNumber);
  }
  else
  {
    emit notepadWriteFailed(pageNumber, QString("Write failed"));
  }
}

void SensorWorker::readNotepad(int pageNumber)
{
  if (!m_sensor)
  {
    emit notepadReadFailed(pageNumber, "Sensor not available");
    return;
  }

  if (pageNumber < 0 || pageNumber > 15)
  {
    emit notepadReadFailed(pageNumber, "Invalid page number (must be 0-15)");
    return;
  }

  auto result = m_sensor->read_notepad(static_cast<std::uint8_t>(pageNumber));

  if (result)
  {
    const auto& data_array = result.value();
    QByteArray qdata(reinterpret_cast<const char*>(data_array.data()), 32);
    emit notepadDataRead(pageNumber, qdata);
  }
  else
  {
    emit notepadReadFailed(pageNumber, QString("Read failed"));
  }
}

void SensorWorker::turnLedOn()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (!m_sensor->turn_led_on())
  {
    emit operationFailed("Failed to turn LED on");
  }
  else
  {
    emit operationComplete("LED turned ON (manual control)");
  }
}

void SensorWorker::turnLedOff()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (!m_sensor->turn_led_off())
  {
    emit operationFailed("Failed to turn LED off");
  }
  else
  {
    emit operationComplete("LED turned OFF (manual control)");
  }
}

void SensorWorker::setBaudRate(int baudChoice)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (baudChoice < 1 || baudChoice > 12)
  {
    emit operationFailed("Invalid baud rate choice. Must be 1-12.");
    return;
  }

  const auto setting = static_cast<carbio::baud_rate_setting>(baudChoice);
  if (!m_sensor->set_baud_rate_setting(setting))
  {
    emit operationFailed("Failed to set baud rate");
  }
  else
  {
    emit operationComplete("Baud rate updated. Reconnect required.");
  }
}

void SensorWorker::setSecurityLevel(int level)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (level < 1 || level > 5)
  {
    emit operationFailed("Invalid security level");
    return;
  }

  auto securityLevel = static_cast<carbio::security_level_setting>(level);
  if (!m_sensor->set_security_level_setting(securityLevel))
  {
    emit operationFailed("Failed to set security level");
  }
  else
  {
    emit operationComplete("Security level updated");
  }
}

void SensorWorker::setPacketSize(int size)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (size < 0 || size > 3)
  {
    emit operationFailed("Invalid packet size");
    return;
  }

  if (!m_sensor->set_packet_length_setting(static_cast<carbio::packet_length_setting>(size)))
  {
    emit operationFailed("Failed to set packet size");
  }
  else
  {
    emit operationComplete("Data packet size updated");
  }
}

void SensorWorker::softResetSensor()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (!m_sensor->soft_reset_device())
  {
    emit operationFailed("Failed to reset sensor");
  }
  else
  {
    emit operationComplete("Sensor reset successfully");
  }
}

void SensorWorker::showSystemSettings()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  auto settings = m_sensor->query_device_settings();
  if (!settings)
  {
    emit operationFailed("Failed to read system parameters");
    return;
  }

  QString settingsInfo;
  settingsInfo += "System Settings:\n";
  settingsInfo += "Library Size: " + QString::number(settings->capacity) + "\n";
  settingsInfo += "Security Level: " + QString::number(settings->security_level) + "\n";
  settingsInfo += "Packet Length: " + QString::number(settings->length) + "\n";
  settingsInfo += "Baud Rate: " + QString::number(settings->baudrate);

  emit operationComplete(settingsInfo);
}

void SensorWorker::resetSensorState()
{
  // Intentionally empty - sensor state is managed by HAL
  // Kept for backward compatibility with controller enrollment cancel flow
}

void SensorWorker::stopAllTimers()
{
  if (m_authPollTimer)
    m_authPollTimer->stop();
  if (m_adminPollTimer)
    m_adminPollTimer->stop();
}
