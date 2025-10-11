#include "sensor_worker.h"
#include "carbio/fingerprint_sensor.h"

// #include <QDebug>
#include <QThread>

SensorWorker::SensorWorker(carbio::fingerprint_sensor* sensor, QObject *parent)
  : QObject(parent)
  , m_sensor(sensor)
  , m_authPollTimer(new QTimer(this))
  , m_adminPollTimer(new QTimer(this))
  , m_operationPollTimer(new QTimer(this))
  , m_currentOperation(OperationType::NONE)
  , m_operationParameter(0)
  , m_enrollmentStage(0)
  , m_cachedSettings(std::nullopt)
{
  // Connect timers to their handlers (all run on worker thread - no context switching!)
  connect(m_authPollTimer, &QTimer::timeout, this, &SensorWorker::onAuthPollTick);
  connect(m_adminPollTimer, &QTimer::timeout, this, &SensorWorker::onAdminPollTick);
  connect(m_operationPollTimer, &QTimer::timeout, this, &SensorWorker::onOperationPollTick);

  // qDebug() << "SensorWorker created with direct polling timers on worker thread";
}

// Pre-warm cache
void SensorWorker::prewarmCache()
{
  if (!m_sensor || m_cachedSettings.has_value())
  {
    return;
  }

  auto settings = m_sensor->get_device_setting_info();
  if (settings)
  {
    m_cachedSettings = settings.value();
    // qDebug() << "Device settings cached - capacity:" << m_cachedSettings->capacity;
  }
}

SensorWorker::~SensorWorker()
{
  // Stop all timers
  m_authPollTimer->stop();
  m_adminPollTimer->stop();
  m_operationPollTimer->stop();

  // qDebug() << "SensorWorker destroyed";
}

// ============================================================================
//  Direct Worker Polling
// ============================================================================

void SensorWorker::startAuthenticationPolling(int intervalMs)
{
  // qInfo() << "Starting DIRECT worker thread polling at" << intervalMs << "ms intervals (zero context switching)";
  m_authPollTimer->setInterval(intervalMs);
  m_authPollTimer->start();
}

void SensorWorker::stopAuthenticationPolling()
{
  // qInfo() << "Stopping authentication polling";
  m_authPollTimer->stop();
  emit scanProgressUpdate(0);
}

void SensorWorker::startAdminPolling(int intervalMs)
{
  // qInfo() << "Starting admin fingerprint polling at" << intervalMs << "ms intervals";
  m_adminPollTimer->setInterval(intervalMs);
  m_adminPollTimer->start();
}

void SensorWorker::stopAdminPolling()
{
  // qInfo() << "Stopping admin polling";
  m_adminPollTimer->stop();
}

void SensorWorker::onAuthPollTick()
{
  // HOT PATH: This runs 333 times per second - every microsecond counts!

  if (!m_sensor) [[unlikely]]
  {
    emit authenticationNoFinger();
    return;
  }

  // Quick check for finger presence (hardware query ~3-5ms)
  auto result = m_sensor->capture_image();

  // No finger detected - continue polling silently
  if (carbio::status_code::no_finger == result.error()) [[likely]]
  {
    // Silently continue - NO logging, NO signals (eliminates overhead)
    return;
  }

  // Transient errors - continue polling without noise
  if (!result) [[unlikely]]
  {
    auto error = result.error();
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_capture_error ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features) [[likely]]
    {
      // Transient error - continue polling silently
      return;
    }

    // Serious error - report it
    emit authenticationFailed();
    return;
  }

  // FINGER DETECTED! Stop polling and complete authentication
  m_authPollTimer->stop();

  // Extract features (~15-25ms)
  auto featuresResult = m_sensor->extract_features(1);
  if (!featuresResult) [[unlikely]]
  {
    auto error = featuresResult.error();
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features) [[likely]]
    {
      // Restart polling on transient error
      m_authPollTimer->start();
      return;
    }

    emit authenticationFailed();
    return;
  }

  // Search for match using cached settings (~10-20ms)
  auto settings = getDeviceSettings();
  auto searchResult = m_sensor->fast_search_model(0, 1, settings.capacity);
  if (!searchResult) [[unlikely]]
  {
    auto error = searchResult.error();
    if (error == carbio::status_code::no_match ||
        error == carbio::status_code::not_found) [[likely]]
    {
      emit authenticationFailed();
      return;
    }

    // Communication error - restart polling
    m_authPollTimer->start();
    return;
  }

  // SUCCESS! Only emit final progress signal
  emit scanProgressUpdate(100);
  emit authenticationSuccess(searchResult->index, searchResult->confidence, "");
}

void SensorWorker::onAdminPollTick()
{
  // HOT PATH: Admin polling at 333 Hz

  if (!m_sensor) [[unlikely]]
  {
    emit adminFingerprintFailed("Sensor not available");
    return;
  }

  // Quick check for finger presence
  auto result = m_sensor->capture_image();

  // MOST COMMON: No finger - continue polling silently
  if (carbio::status_code::no_finger == result.error()) [[likely]]
  {
    return;
  }

  // Transient errors - continue polling silently
  if (!result) [[unlikely]]
  {
    auto error = result.error();
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_capture_error ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features) [[likely]]
    {
      return;
    }

    emit adminFingerprintFailed("Image capture failed");
    return;
  }

  // FINGER DETECTED! Stop polling and complete authentication
  m_adminPollTimer->stop();

  // Extract features
  auto featuresResult = m_sensor->extract_features(1);
  if (!featuresResult) [[unlikely]]
  {
    auto error = featuresResult.error();
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features) [[likely]]
    {
      // Restart polling on transient error
      m_adminPollTimer->start();
      return;
    }

    emit adminFingerprintFailed("Feature extraction failed");
    return;
  }

  // Search for match using cached settings
  auto settings = getDeviceSettings();
  auto searchResult = m_sensor->fast_search_model(0, 1, settings.capacity);
  if (!searchResult) [[unlikely]]
  {
    auto error = searchResult.error();
    if (error == carbio::status_code::no_match ||
        error == carbio::status_code::not_found) [[likely]]
    {
      emit adminFingerprintFailed("Fingerprint not recognized");
      return;
    }

    // Communication error - restart polling
    m_adminPollTimer->start();
    return;
  }

  // SUCCESS!
  emit adminFingerprintSuccess(searchResult->index, searchResult->confidence);
}

// ============================================================================
// Legacy single-shot methods (kept for compatibility)
// ============================================================================

void SensorWorker::performAuthentication()
{
  if (!m_sensor)
  {
    emit authenticationFailed();
    return;
  }

  // Step 1: Check if finger is present (non-blocking) - 0-20%
  emit scanProgressUpdate(5);
  auto result = m_sensor->capture_image();

  // Handle no finger - don't count as failure
  if (carbio::status_code::no_finger == result.error())
  {
    emit authenticationNoFinger();
    return;
  }

  // Handle capture errors - distinguish between transient and authentication failures
  if (!result)
  {
    auto error = result.error();
    // qWarning() << "Failed to capture image:" << carbio::get_message(error);

    // Transient/hardware errors - don't count as authentication failure
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_capture_error ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features)
    {
      // Treat as "no finger" - allows retry without incrementing failed attempts
      emit authenticationNoFinger();
      return;
    }

    // Other errors count as authentication failure
    emit authenticationFailed();
    return;
  }

  // qInfo() << "Image captured successfully";
  emit scanProgressUpdate(30);
  emit progressUpdate("Finger detected - capturing...");

  // Step 2: Extract fingerprint features - 20-60% (immediate - no delay to prevent finger movement)
  emit scanProgressUpdate(40);
  auto featuresResult = m_sensor->extract_features(1);
  if (!featuresResult)
  {
    auto error = featuresResult.error();
    // qWarning() << "Failed to create template:" << carbio::get_message(error);

    // Transient/hardware errors during feature extraction - don't count as authentication failure
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features)
    {
      // Treat as "no finger" - allows retry without incrementing failed attempts
      emit authenticationNoFinger();
      return;
    }

    // Other errors count as authentication failure
    emit authenticationFailed();
    return;
  }

  // qInfo() << "Template created successfully";
  emit scanProgressUpdate(65);
  emit progressUpdate("Processing fingerprint...");

  // Step 3: Search for match - 60-95%
  emit scanProgressUpdate(75);
  auto settings = m_sensor->get_device_setting_info();
  auto searchResult = m_sensor->fast_search_model(0, 1, settings->capacity);
  if (!searchResult)
  {
    auto error = searchResult.error();

    // Only no_match and not_found are legitimate authentication failures
    if (error == carbio::status_code::no_match ||
        error == carbio::status_code::not_found)
    {
      // qWarning() << "Fingerprint not found in database";
      emit authenticationFailed();
      return;
    }

    // All other errors (communication, hardware) - don't count as authentication failure
    // qWarning() << "Search failed with error:" << carbio::get_message(error);
    emit authenticationNoFinger();
    return;
  }

  // Success! - 95-100%
  // qInfo() << "Authentication successful. Finger index: " << searchResult->index << ", confidence:" << searchResult->confidence;
  emit scanProgressUpdate(95);
  emit progressUpdate("Verifying identity...");
  emit authenticationSuccess(searchResult->index, searchResult->confidence, "");
}

void SensorWorker::performAdminAuthentication()
{
  if (!m_sensor)
  {
    emit adminFingerprintFailed("Sensor not available");
    return;
  }

  // Step 1: Check if finger is present (non-blocking)
  auto result = m_sensor->capture_image();

  // Handle no finger - don't count as failure
  if (carbio::status_code::no_finger == result.error())
  {
    emit adminFingerprintNoFinger();
    return;
  }

  // Handle capture errors - distinguish between transient and authentication failures
  if (!result)
  {
    auto error = result.error();
    // qWarning() << "Failed to capture image:" << carbio::get_message(error);

    // Transient/hardware errors - don't count as authentication failure
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_capture_error ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features)
    {
      // Treat as "no finger" - allows retry without incrementing failed attempts
      emit adminFingerprintNoFinger();
      return;
    }

    // Other errors count as authentication failure
    emit adminFingerprintFailed("Image capture failed");
    return;
  }

  // qInfo() << "Admin auth: Image captured successfully";

  // Step 2: Extract fingerprint features
  auto featuresResult = m_sensor->extract_features(1);
  if (!featuresResult)
  {
    auto error = featuresResult.error();
    // qWarning() << "Failed to create template:" << carbio::get_message(error);

    // Transient/hardware errors during feature extraction - don't count as authentication failure
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features)
    {
      // Treat as "no finger" - allows retry without incrementing failed attempts
      emit adminFingerprintNoFinger();
      return;
    }

    // Other errors count as authentication failure
    emit adminFingerprintFailed("Feature extraction failed");
    return;
  }

  // qInfo() << "Admin auth: Template created successfully";

  // Step 3: Search for match
  auto settings = m_sensor->get_device_setting_info();
  auto searchResult = m_sensor->fast_search_model(0, 1, settings->capacity);
  if (!searchResult)
  {
    auto error = searchResult.error();

    // Only no_match and not_found are legitimate authentication failures
    if (error == carbio::status_code::no_match ||
        error == carbio::status_code::not_found)
    {
      // qWarning() << "Admin auth: Fingerprint not found in database";
      emit adminFingerprintFailed("Fingerprint not recognized");
      return;
    }

    // All other errors (communication, hardware) - don't count as authentication failure
    // qWarning() << "Admin auth: Search failed with error:" << carbio::get_message(error);
    emit adminFingerprintNoFinger();
    return;
  }

  // Success!
  // qInfo() << "Admin auth successful. Finger index: " << searchResult->index << ", confidence:" << searchResult->confidence;
  emit adminFingerprintSuccess(searchResult->index, searchResult->confidence);
}

void SensorWorker::enrollFingerprint(int id)
{
  if (!m_sensor)
  {
    emit enrollmentFailed("Sensor not available");
    return;
  }

  // Validate ID range
  if (id < 1 || id > 127)
  {
    emit enrollmentFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  // qDebug() << "Starting ASYNC enrollment for ID:" << id << "(non-blocking, multi-stage)";
  resetSensorState();

  // Setup async multi-stage enrollment
  emit scanProgressUpdate(0);
  emit enrollmentProgress("Place finger on sensor...");

  m_currentOperation = OperationType::ENROLL;
  m_operationParameter = id;
  m_enrollmentStage = 0;  // Start at stage 0 (first capture)

  // Start polling
  m_operationPollTimer->setInterval(3);
  m_operationPollTimer->start();
}

// ============================================================================
// Async Dialog Operations (non-blocking with polling)
// ============================================================================

void SensorWorker::onOperationPollTick()
{
  // This runs on worker thread - handles async polling for dialog operations

  if (!m_sensor)
  {
    m_operationPollTimer->stop();
    emit operationFailed("Sensor not available");
    return;
  }

  // Try to capture image
  auto result = m_sensor->capture_image();

  // No finger - continue polling silently (provides responsive UX)
  if (carbio::status_code::no_finger == result.error())
  {
    return;  // Keep polling
  }

  // Transient errors - continue polling
  if (!result)
  {
    auto error = result.error();
    if (error == carbio::status_code::frame_error ||
        error == carbio::status_code::timeout ||
        error == carbio::status_code::bad_packet ||
        error == carbio::status_code::communication_error ||
        error == carbio::status_code::hardware_fault ||
        error == carbio::status_code::image_capture_error ||
        error == carbio::status_code::image_too_faint ||
        error == carbio::status_code::image_too_blurry ||
        error == carbio::status_code::image_too_distorted ||
        error == carbio::status_code::image_too_few_features)
    {
      return;  // Keep polling
    }

    // Serious error
    m_operationPollTimer->stop();
    emit operationFailed("Failed to capture image");
    return;
  }

  // Finger detected! Stop polling and complete operation based on type
  m_operationPollTimer->stop();

  switch (m_currentOperation)
  {
  case OperationType::NONE:
    // No operation in progress - should not happen, but handle gracefully
    // qWarning() << "Finger detected but no operation in progress";
    break;
  case OperationType::FIND:
    completeFindOperation();
    break;
  case OperationType::IDENTIFY:
    completeIdentifyOperation();
    break;
  case OperationType::VERIFY:
    completeVerifyOperation(m_operationParameter);
    break;
  case OperationType::ENROLL:
    continueEnrollmentOperation(m_operationParameter);
    break;
  default:
    // All enum values are handled above, but default required by -Wswitch-default
    // qWarning() << "Unknown operation type in switch";
    break;
  }

  m_currentOperation = OperationType::NONE;
}

void SensorWorker::completeFindOperation()
{
  emit progressUpdate("Processing fingerprint...");

  if (!m_sensor->extract_features(1))
  {
    emit operationFailed("Failed to create template");
    return;
  }

  auto settings = getDeviceSettings();
  auto result = m_sensor->fast_search_model(0, 1, settings.capacity);
  if (!result)
  {
    emit operationFailed("Fingerprint not found in database");
    return;
  }

  emit operationComplete("Found: ID #" + QString::number(result->index) +
                        ", confidence: " + QString::number(result->confidence) + ")");
}

void SensorWorker::completeIdentifyOperation()
{
  emit progressUpdate("Identifying fingerprint...");

  if (!m_sensor->extract_features(1))
  {
    emit operationFailed("Failed to create template");
    return;
  }

  auto settings = getDeviceSettings();
  auto result = m_sensor->fast_search_model(0, 1, settings.capacity);
  if (!result)
  {
    emit operationFailed("No matching fingerprint found");
    return;
  }

  emit operationComplete("ID #" + QString::number(result->index) +
                        ", confidence: " + QString::number(result->confidence));
}

void SensorWorker::completeVerifyOperation(int id)
{
  emit progressUpdate("Verifying fingerprint...");

  if (!m_sensor->extract_features(1))
  {
    emit operationFailed("Failed to create template");
    return;
  }

  // Load stored template into buffer 2
  if (!m_sensor->load_model(static_cast<uint16_t>(id), 2))
  {
    emit operationFailed("Template #" + QString::number(id) + " not found");
    return;
  }

  // Search only for this specific ID
  auto result = m_sensor->fast_search_model(static_cast<uint16_t>(id), 1, 1);
  if (!result || result->index != static_cast<uint16_t>(id))
  {
    emit operationFailed("Fingerprint does NOT match ID #" + QString::number(id));
  }
  else
  {
    emit operationComplete("VERIFIED - ID #" + QString::number(id) +
                          " (confidence: " + QString::number(result->confidence) + ")");
  }
}

void SensorWorker::continueEnrollmentOperation(int id)
{
  // Enrollment is multi-stage, handle based on current stage
  if (m_enrollmentStage == 0)
  {
    // First capture already done (from polling), extract features
    emit scanProgressUpdate(15);
    emit enrollmentProgress("Capturing first scan...");

    if (!m_sensor->extract_features(1))
    {
      resetSensorState();
      emit scanProgressUpdate(0);
      emit enrollmentFailed("Failed to create first template");
      return;
    }

    emit scanProgressUpdate(25);
    emit enrollmentProgress("Remove finger, then place again...");
    m_enrollmentStage = 1;

    // Start polling for second capture
    m_currentOperation = OperationType::ENROLL;
    m_operationPollTimer->start();
  }
  else if (m_enrollmentStage == 1)
  {
    // Second capture already done, extract features
    emit scanProgressUpdate(40);
    emit enrollmentProgress("Capturing second scan...");

    if (!m_sensor->extract_features(2))
    {
      resetSensorState();
      emit scanProgressUpdate(0);
      emit enrollmentFailed("Failed to create second template");
      return;
    }

    // Create model
    emit scanProgressUpdate(50);
    emit enrollmentProgress("Creating fingerprint model...");

    if (!m_sensor->create_model())
    {
      resetSensorState();
      emit scanProgressUpdate(0);
      emit enrollmentFailed("Failed to create fingerprint model");
      return;
    }

    // Store template
    emit scanProgressUpdate(75);
    emit enrollmentProgress("Storing template...");

    if (!m_sensor->store_model(static_cast<uint16_t>(id)))
    {
      resetSensorState();
      emit scanProgressUpdate(0);
      emit enrollmentFailed("Failed to store template");
      return;
    }

    emit scanProgressUpdate(100);
    emit enrollmentComplete("Fingerprint enrolled successfully as ID #" + QString::number(id));
    m_enrollmentStage = 0;
  }
}

void SensorWorker::findFingerprint()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Starting ASYNC find fingerprint operation (non-blocking)";
  emit progressUpdate("Place finger on sensor...");

  // Setup async operation
  m_currentOperation = OperationType::FIND;
  m_operationParameter = 0;

  // Start polling
  m_operationPollTimer->setInterval(3);
  m_operationPollTimer->start();
}

void SensorWorker::identifyFingerprint()
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Starting ASYNC identify fingerprint operation (non-blocking)";
  emit progressUpdate("Place finger on sensor...");

  // Setup async operation
  m_currentOperation = OperationType::IDENTIFY;
  m_operationParameter = 0;

  // Start polling
  m_operationPollTimer->setInterval(3);
  m_operationPollTimer->start();
}

void SensorWorker::verifyFingerprint(int id)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Starting ASYNC verify fingerprint operation for ID:" << id << "(non-blocking)";
  emit progressUpdate("Place finger on sensor...");

  // Setup async operation
  m_currentOperation = OperationType::VERIFY;
  m_operationParameter = id;

  // Start polling
  m_operationPollTimer->setInterval(3);
  m_operationPollTimer->start();
}

void SensorWorker::queryTemplate(int id)
{
  if (!m_sensor)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Querying template for ID:" << id;

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

  // qDebug() << "Deleting fingerprint for ID:" << id;

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

  // qDebug() << "Clearing fingerprint database";

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

  // Validate baud choice range
  if (baudChoice < 1 || baudChoice > 12)
  {
    emit operationFailed("Invalid baud rate choice. Must be 1-12.");
    return;
  }

  // qDebug() << "Setting baud rate:" << baudChoice;

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

  auto packetSize = static_cast<carbio::data_length_setting>(size);
  if (!m_sensor->set_data_length_setting(packetSize))
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

  // qDebug() << "Soft resetting sensor";

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

  auto settings = m_sensor->get_device_setting_info();
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
  if (!m_sensor)
  {
    return;
  }

  // qDebug() << "Resetting sensor state";
  // Try to clear any pending operations
  static_cast<void>(m_sensor->capture_image()); // Clear image buffer
}
