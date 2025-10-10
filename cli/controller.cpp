#include "controller.h"

#include "carbio/fingerprint_sensor.h"

#include <QDebug>
#include <QThread>
#include <cstdlib>

Controller::Controller(QObject *parent)
  : QObject(parent)
  , m_sensor(std::make_unique<carbio::fingerprint_sensor>())
  , m_authState(AuthState::OFF)
  , m_driverName("")
  , m_failedAttempts(0)
  , m_lockoutSeconds(0)
  , m_sensorAvailable(false)
  , m_isProcessing(false)
  , m_templateCount(0)
  , m_operationProgress("")
  , m_manualLedControl(false)
  , m_isAdminMenuAccessible(false)
  , m_adminAccessToken("")
  , m_scanTimer(new QTimer(this))
  , m_lockoutTimer(new QTimer(this))
  , m_adminFingerprintTimer(new QTimer(this))
  , m_consecutiveNoFingerAuth(0)
  , m_consecutiveNoFingerAdmin(0)
  , m_adminAuth(std::make_unique<carbio::security::AdminAuthenticator>(this))
  , m_sessionManager(std::make_unique<carbio::security::SessionManager>(this))
  , m_auditLogger(std::make_unique<carbio::security::AuditLogger>(this))
  , m_profileManager(std::make_unique<carbio::security::ProfileManager>(this))
  , m_adminAuthPhase(AdminAuthPhase::IDLE)
{
  // Setup adaptive scanning timer - starts at 50ms, scales to 1000ms for power efficiency
  m_scanTimer->setInterval(POLL_INTERVAL_MIN);
  connect(m_scanTimer, &QTimer::timeout, this, &Controller::performAuthentication);

  // Setup lockout countdown timer (1 second ticks)
  m_lockoutTimer->setInterval(1000);
  connect(m_lockoutTimer, &QTimer::timeout, this, &Controller::onLockoutTick);

  // Setup admin fingerprint polling timer with adaptive interval
  m_adminFingerprintTimer->setInterval(POLL_INTERVAL_MIN);
  connect(m_adminFingerprintTimer, &QTimer::timeout, this, &Controller::onAdminFingerprintPoll);

  // Connect admin auth signals
  connect(m_adminAuth.get(), &carbio::security::AdminAuthenticator::passwordVerified, this, &Controller::adminPasswordVerified);
  connect(m_adminAuth.get(), &carbio::security::AdminAuthenticator::passwordFailed, this, &Controller::adminPasswordFailed);
  connect(m_auditLogger.get(), &carbio::security::AuditLogger::unauthorizedAccessDetected, this, &Controller::unauthorizedAccessDetected);

  // Load user profiles
  if (!m_profileManager->loadProfiles())
  {
    qWarning() << "Failed to load profiles, starting with empty profile list";
  }

  qDebug() << "Controller initialized with security components";
}

Controller::~Controller() = default;

bool Controller::initializeSensor()
{
  try
  {
    qDebug() << "Attempting to initialize fingerprint sensor...";

    // Check for environment variable to override serial port
    const char* portEnv = std::getenv("FINGERPRINT_PORT");
    const char* port = portEnv ? portEnv : "/dev/ttyAMA0";

    qInfo() << "Using serial port:" << port;
    if (m_sensor->open(port))
    {
      m_sensorAvailable = true;
      emit sensorAvailableChanged();
      refreshTemplateCount();
      qInfo() << "Starting authentication scanning";
      startAuthentication();
      return true;
    }
    else
    {
      qWarning() << "Failed to initialize sensor - running in demo mode";
      m_sensorAvailable = false;
      emit sensorAvailableChanged();
      return false;
    }
  }
  catch (std::exception const &e)
  {
    qCritical() << "Sensor initialization exception:" << e.what();
    m_sensorAvailable = false;
    emit sensorAvailableChanged();
    return false;
  }
  catch (...)
  {
    qCritical() << "Unknown exception during sensor initialization";
    m_sensorAvailable = false;
    emit sensorAvailableChanged();
    return false;
  }
}

void Controller::startAuthentication()
{
  if (!m_sensorAvailable)
  {
    qWarning() << "Sensor not available";
    return;
  }

  if (m_authState == AuthState::ALERT)
  {
    qWarning() << "System locked out";
    return;
  }

  setAuthState(AuthState::SCANNING);

  // Turn on LED to indicate scanning mode
  if (!m_sensor->turn_led_on())
  {
    qWarning() << "Failed to turn on LED";
  }

  // Reset adaptive polling to responsive mode
  resetAuthPollingInterval();

  // Start adaptive scanning
  m_scanTimer->start();
}

void Controller::cancelAuthentication()
{
  m_scanTimer->stop();

  // Reset polling interval for next time authentication starts
  resetAuthPollingInterval();

  if (!m_manualLedControl)
  {
    if (!m_sensor->turn_led_off())
    {
      qWarning() << "Failed to turn off LED";
    }
  }

  setAuthState(AuthState::OFF);
}

void Controller::resetLockout()
{
  m_lockoutTimer->stop();
  m_lockoutSeconds  = 0;
  m_failedAttempts = 0;
  emit lockoutSecondsChanged();
  emit failedAttemptsChanged();

  if (m_sensor)
  {
    // Turn on LED to indicate ready state
    if (!m_sensor->turn_led_off())
    {
      qWarning() << "Failed to turn on LED";
    }
  }

  setAuthState(AuthState::SCANNING);

  // Reset adaptive polling to responsive mode
  resetAuthPollingInterval();

  m_scanTimer->start();
}

void Controller::performAuthentication()
{
  if (m_authState != AuthState::SCANNING)
  {
    return;
  }

  // Step 1: Check if finger is present (non-blocking)
  {
    auto result = m_sensor->capture_image();
    if (carbio::status_code::no_finger == result.error())
    {
      // If no finger, implement adaptive polling with exponential backoff
      m_consecutiveNoFingerAuth++;
      // adjust polling interval exponentially to save CPU cycles
      adjustAuthPollingInterval();
      return;
    }

    // Finger detected! Reset adaptive polling and start authentication
    resetAuthPollingInterval();

    // Stop polling and start authentication
    if (m_scanTimer->isActive())
    {
      m_scanTimer->stop();
    }

    setIsProcessing(true);
    setAuthState(AuthState::AUTHENTICATING);
    if (!result)
    {
      qWarning() << "Failed to capture image:" << carbio::get_message(result.error());
      handleAuthenticationFailure();
      return;
    }

    qInfo() << "Image captured successfully";
  }
  // Step 2: Extract fingerprint features
  {
    auto result = m_sensor->extract_features(1);
    if (!result)
    {
      qWarning() << "Failed to create template:" << carbio::get_message(result.error());
      handleAuthenticationFailure();
      return;
    }
    qInfo() << "Template created successfully";
  }
  // Step 3: Search for match
  {
    auto settings = m_sensor->get_device_setting_info();
    auto result = m_sensor->fast_search_model(0, 1, settings->capacity);
    if (!result)
    {
      qWarning() << "Fingerprint not found in database";
      handleAuthenticationFailure();
      return;
    }

    // Success!
    qInfo() << "Authentication successful. Finger index: " << result->index << ", confidence:" << result->confidence;
    m_scanTimer->stop();
    m_failedAttempts = 0;
    emit failedAttemptsChanged();

    m_driverName = lookupDriverName(result->index);
    emit driverNameChanged();

    // Turn off LED on success (dashboard is now visible)
    if (!m_sensor->turn_led_off())
    {
      qWarning() << "Failed to turn off LED";
    }
  }
  setIsProcessing(false);
  emit authenticationSuccess(m_driverName);
  setAuthState(AuthState::ON);
}

void Controller::handleAuthenticationFailure()
{
  setIsProcessing(false);
  m_failedAttempts++;
  emit failedAttemptsChanged();

  if (m_failedAttempts >= MAX_ATTEMPTS)
  {
    // Trigger lockout - stop polling to save CPU/power
    m_scanTimer->stop();

    m_lockoutSeconds = LOCKOUT_DURATION_SEC;
    emit lockoutSecondsChanged();
    setAuthState(AuthState::ALERT);
    m_lockoutTimer->start();

    // Keep LED on during lockout as warning
    static_cast<void>(m_sensor->turn_led_on());

    emit lockoutTriggered();
  }
  else
  {
    emit authenticationFailed();


    // Return to scanning mode and restart polling with reset interval
    setAuthState(AuthState::SCANNING);
    resetAuthPollingInterval();
    m_scanTimer->start();
  }
}

QString Controller::lookupDriverName(uint16_t fingerId)
{
  // Use profile manager to look up driver name
  return m_profileManager->getDriverName(fingerId);
}

void Controller::setAuthState(AuthState state)
{
  if (m_authState != state)
  {
    AuthState previousState = m_authState;
    m_authState = state;
    qDebug() << "Auth state changed to:" << authStateToString(state).data();
    emit authStateChanged();
    
    if (state == AuthState::SCANNING && previousState == AuthState::ON && m_failedAttempts > 0)
    {
      qInfo() << "Resetting failed attempts from" << m_failedAttempts << "to 0 (entering SCANNING from ON)";
      m_failedAttempts = 0;
      emit failedAttemptsChanged();
    }
  }
}

void Controller::setIsProcessing(bool processing)
{
  if (m_isProcessing != processing)
  {
    m_isProcessing = processing;
    qDebug() << "Processing state changed to:" << (processing ? "true" : "false");
    emit isProcessingChanged();
  }
}

void Controller::setTemplateCount(int count)
{
  if (m_templateCount != count)
  {
    m_templateCount = count;
    qDebug() << "Template count:" << count;
    emit templateCountChanged();
  }
}

void Controller::setOperationProgress(const QString &progress)
{
  if (m_operationProgress != progress)
  {
    m_operationProgress = progress;
    emit operationProgressChanged();
    emit operationComplete(progress);
  }
}

void Controller::resetSensorState()
{
  if (!m_sensor)
  {
    return;
  }

  qDebug() << "Resetting sensor state";
  // Try to clear any pending operations
  static_cast<void>(m_sensor->capture_image()); // Clear image buffer
}

void Controller::refreshTemplateCount()
{
  if (!m_sensorAvailable)
  {
    setTemplateCount(0);
    return;
  }

  auto result = m_sensor->model_count();
  if (result)
  {
    setTemplateCount(static_cast<int>(result.value()));
  }
  else
  {
    setTemplateCount(0);
  }
}

void Controller::onLockoutTick()
{
  if (m_lockoutSeconds > 0)
  {
    m_lockoutSeconds--;
    emit lockoutSecondsChanged();

    if (m_lockoutSeconds <= 0)
    {
      m_lockoutTimer->stop();
      
      qInfo() << "Lockout expired. Resetting failed attempts from" << m_failedAttempts << "to 0";

      m_failedAttempts = 0;
      emit failedAttemptsChanged();
      
      qInfo() << "Failed attempts reset complete. Value is now:" << m_failedAttempts;

      if (m_sensor)
      {
        // Turn off LED when lockout expires
        if (!m_sensor->turn_led_off())
        {
          qWarning() << "Failed to turn off LED";
        }
      }

      // Now transition to SCANNING state with reset polling
      setAuthState(AuthState::SCANNING);
      resetAuthPollingInterval();
      startAuthentication();
    }
  }
}

void Controller::enrollFingerprint(int id)
{
  // Validate sensor availability
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // Validate that authentication is not running
  if (m_scanTimer->isActive())
  {
    emit operationFailed("Cannot enroll while authentication is active. Close settings first.");
    return;
  }

  // Validate ID range
  if (id < 1 || id > 127)
  {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  qDebug() << "Enrolling fingerprint for ID:" << id;
  setIsProcessing(true);
  resetSensorState();

  // Step 1: Capture first image
  {
    setOperationProgress("Place finger on sensor...");
    if (!m_sensor->capture_image())
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to capture first image");
      return;
    }

    if (!m_sensor->extract_features(1))
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to create first template");
      return;
    }

    setOperationProgress("Remove finger...");
    QThread::msleep(50);
  }
  // Step 2: Capture second image
  {
    setOperationProgress("Place same finger again...");
    if (!m_sensor->capture_image())
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to capture second image");
      return;
    }

    if (!m_sensor->extract_features(2))
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to create second template");
      return;
    }
  }

  // Step 3: Create model from image samples
  {
    setOperationProgress("Creating fingerprint model...");
    if (!m_sensor->create_model())
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to create fingerprint model");
      return;
    }
  }

  // Step 4: Store in secure data store
  {
    setOperationProgress("Storing template...");
    if (!m_sensor->store_model(static_cast<uint16_t>(id)))
    {
      resetSensorState();
      setIsProcessing(false);
      emit operationFailed("Failed to store template");
      return;
    }
  }
  setIsProcessing(false);
  refreshTemplateCount();
  emit operationComplete("Fingerprint enrolled successfully as ID #" + QString::number(id));
}

void Controller::findFingerprint()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Finding fingerprint";
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  if (!m_sensor->capture_image())
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  if (!m_sensor->extract_features(1))
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  {
    auto settings = m_sensor->get_device_setting_info();
    auto result = m_sensor->fast_search_model(0, 1, settings->capacity);
    if (!result)
    {
      setIsProcessing(false);
      emit operationFailed("Fingerprint not found in database");
      return;
    }
    else
    {
      setIsProcessing(false);
      emit operationComplete("Found: ID #" + QString::number(result->index) +", confidence: " + QString::number(result->confidence) + ")");
    }
  }
}

void Controller::identifyFingerprint()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Identifying fingerprint";
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  if (!m_sensor->capture_image())
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  if (!m_sensor->extract_features(1))
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  auto settings = m_sensor->get_device_setting_info();
  auto result = m_sensor->fast_search_model(0, 1, settings->capacity);
  if (!result)
  {
    setIsProcessing(false);
    emit operationFailed("No matching fingerprint found");
    return;
  }
  else
  {
    QString driverName = lookupDriverName(result->index);
    setIsProcessing(false);
    emit operationComplete("Identified: " + driverName + " [ID #" + QString::number(result->index) + ", confidence: " + QString::number(result->confidence) + "]");
  }
}

void Controller::verifyFingerprint(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Verifying fingerprint for ID:" << id;
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  // Capture image and create template
  if (!m_sensor->capture_image())
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  if (!m_sensor->extract_features(1))
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  // Load stored template into buffer 2
  if (! m_sensor->load_model(static_cast<uint16_t>(id), 2))
  {
    setIsProcessing(false);
    emit operationFailed("Template #" + QString::number(id) + " not found");
    return;
  }

  // Search only for this specific ID
  auto result = m_sensor->fast_search_model(static_cast<uint16_t>(id), 1, 1);
  if (!result || result->index != static_cast<uint16_t>(id))
  {
    setIsProcessing(false);
    emit operationFailed("Fingerprint does NOT match ID #" + QString::number(id));
  }
  else
  {
    setIsProcessing(false);
    emit operationComplete("VERIFIED - ID #" + QString::number(id) + " (confidence: " + QString::number(result->confidence) + ")");
  }
}

void Controller::queryTemplate(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Querying template for ID:" << id;

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

void Controller::deleteFingerprint(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_scanTimer->isActive())
  {
    emit operationFailed("Cannot delete while authentication is active");
    return;
  }

  if (id < 1 || id > 127)
  {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  qDebug() << "Deleting fingerprint for ID:" << id;
  setIsProcessing(true);

  if (!m_sensor->erase_model(static_cast<uint16_t>(id), 1))
  {
    setIsProcessing(false);
    refreshTemplateCount();
    emit operationComplete("Fingerprint #" + QString::number(id) + " deleted");
  }
  else
  {
    setIsProcessing(false);
    emit operationFailed("Failed to delete fingerprint #" + QString::number(id));
  }
}

void Controller::clearDatabase()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_scanTimer->isActive())
  {
    emit operationFailed("Cannot clear database while authentication is active");
    return;
  }

  qDebug() << "Clearing fingerprint database";
  setIsProcessing(true);

  if (!m_sensor->clear_database())
  {
    setIsProcessing(false);
    refreshTemplateCount();
    emit operationComplete("All fingerprints deleted");
  }
  else
  {
    setIsProcessing(false);
    emit operationFailed("Failed to clear database");
  }
}

void Controller::turnLedOn()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  m_manualLedControl = true;
  if (!m_sensor->turn_led_on())
  {
    m_manualLedControl = false;
    emit operationFailed("Failed to turn LED on");
  }
  else
  {
    emit operationComplete("LED turned ON (manual control)");
  }
}

void Controller::turnLedOff()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  m_manualLedControl = true;
  if (!m_sensor->turn_led_off())
  {
    m_manualLedControl = false;
    emit operationFailed("Failed to turn LED off");
  }
  else
  {
    emit operationComplete("LED turned OFF (manual control)");
  }
}

void Controller::toggleLed()
{
    (!m_sensor || !m_sensorAvailable)
    ? emit operationFailed("Sensor not available")
    : (m_sensor->turn_led_off()
         ? emit operationComplete("LED turned OFF")
         : (m_sensor->turn_led_on()
              ? emit operationComplete("LED turned ON")
              : emit operationFailed("Failed to toggle LED")));
}

void Controller::setBaudRate(int baudChoice)
{
  if (!m_sensorAvailable)
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

  qDebug() << "Setting baud rate:" << baudChoice;

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

void Controller::setSecurityLevel(int level)
{
  if (!m_sensorAvailable)
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

void Controller::setPacketSize(int size)
{
  if (!m_sensorAvailable)
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

void Controller::softResetSensor()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Soft resetting sensor";

  if (!m_sensor->soft_reset_device())
  {
    emit operationFailed("Failed to reset sensor");
  }
  else
  {
    emit operationComplete("Sensor reset successfully");
  }
}

void Controller::showSystemSettings()
{
  if (!m_sensorAvailable)
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

// ============================================================================
// Admin Authentication Methods
// ============================================================================

void Controller::requestAdminAccess()
{
  qInfo() << "Admin access requested";

  // Check rate limiting
  if (m_adminAuth->isRateLimited())
  {
    int remaining = m_adminAuth->lockoutSecondsRemaining();
    QString reason = QString("Too many failed attempts. Try again in %1 seconds.").arg(remaining);

    m_auditLogger->logEvent(carbio::security::SecurityEvent::RATE_LIMIT_EXCEEDED, 0,
                            carbio::security::AuthResult::RATE_LIMITED, reason);

    emit adminAccessDenied(reason);
    return;
  }

  // Reset state
  m_adminAuthPhase = AdminAuthPhase::PASSWORD_PENDING;
  setAdminMenuAccessible(false);
  setAdminAccessToken("");

  // Log attempt
  m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_ATTEMPT, 0,
                          carbio::security::AuthResult::SUCCESS, "Admin access request initiated");

  // Request password from user
  emit adminPasswordRequired();
}

void Controller::verifyAdminPassword(const QString &password)
{
  if (m_adminAuthPhase != AdminAuthPhase::PASSWORD_PENDING)
  {
    qWarning() << "Password verification called in wrong phase";
    return;
  }

  qInfo() << "Verifying admin password";

  bool valid = m_adminAuth->verifyPassword(password);
  if (valid)
  {
    qInfo() << "Password verified";
    m_auditLogger->logEvent(carbio::security::SecurityEvent::PASSWORD_VERIFIED, 0, carbio::security::AuthResult::SUCCESS, "Admin password correct");
    m_adminAuthPhase = AdminAuthPhase::FINGERPRINT_PENDING;
    emit adminFingerprintRequired();
  }
  else
  {
    qWarning() << "Password verification failed";
    
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logAdminAccess(0, true, false, false);
    m_auditLogger->logUnauthorizedAccess(0, "Non-admin fingerprint attempted admin access");
    
    lockDashboardAfterAdminFailure();
    
    emit unauthorizedAccessDetected("Unauthorized admin access attempt detected. This incident has been logged.");
    emit adminAccessDenied("Incorrect password");
    return;
  }
}

void Controller::startAdminFingerprintScan()
{
  if (m_adminAuthPhase != AdminAuthPhase::FINGERPRINT_PENDING)
  {
    qWarning() << "Fingerprint scan called in wrong phase";
    emit adminAccessDenied("Invalid authentication flow");
    return;
  }

  if (!m_sensorAvailable)
  {
    emit adminAccessDenied("Sensor not available");
    return;
  }

  qInfo() << "Starting admin fingerprint scan - waiting for finger...";
  setIsProcessing(true);

  // Reset adaptive polling to responsive mode
  resetAdminPollingInterval();

  // Start polling for finger presence
  m_adminFingerprintTimer->start();
}

void Controller::onAdminFingerprintPoll()
{
  if (m_adminAuthPhase != AdminAuthPhase::FINGERPRINT_PENDING)
  {
    m_adminFingerprintTimer->stop();
    return;
  }

  if (!m_sensorAvailable)
  {
    m_adminFingerprintTimer->stop();
    setIsProcessing(false);
    emit adminAccessDenied("Sensor not available");
    return;
  }

  // Check if finger is present (non-blocking poll)
  auto status = m_sensor->capture_image();

  // If no finger, implement adaptive polling with exponential backoff
  if (status.error() == carbio::status_code::no_finger)
  {
    m_consecutiveNoFingerAdmin++;
    adjustAdminPollingInterval();
    return;
  }

  // Finger detected! Reset adaptive polling and stop polling
  resetAdminPollingInterval();
  m_adminFingerprintTimer->stop();

  // Check if image capture was successful
  if (!status)
  {
    qWarning() << "Failed to capture fingerprint during admin access - initiating security lockdown";
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;
    m_auditLogger->logUnauthorizedAccess(0, "Failed to capture fingerprint during admin access");
    lockDashboardAfterAdminFailure();
    emit adminAccessDenied("Failed to capture fingerprint");
    return;
  }

  qInfo() << "Admin finger detected and image captured - verifying...";
  performAdminFingerprintVerification();
}

void Controller::performAdminFingerprintVerification()
{
  // Convert captured image to template
  auto status = m_sensor->extract_features(1);
  if (!status)
  {
    qWarning() << "Failed to create template during admin access - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logUnauthorizedAccess(0, "Failed to create template during admin access");
    lockDashboardAfterAdminFailure();

    emit adminAccessDenied("Failed to process fingerprint");
    return;
  }

  // Search for match
  auto settings = m_sensor->get_device_setting_info();
  auto result = m_sensor->fast_search_model(0, 1, settings->capacity);
  if (!result)
  {
    qWarning() << "Fingerprint NOT FOUND during admin access - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logAdminAccess(0, true, false, false);
    m_auditLogger->logUnauthorizedAccess(0, "Non-admin fingerprint attempted admin access");

    lockDashboardAfterAdminFailure();

    emit unauthorizedAccessDetected("Unauthorized admin access attempt detected. This incident has been logged.");
    emit adminAccessDenied("Fingerprint not recognized");
    return;
  }

  qInfo() << "Fingerprint matched - index: " << result->index << ", confidence:" << result->confidence;

  // Check if fingerprint is admin (ID 0-2)
  if (!isAdminFingerprint(result->index))
  {
    qWarning() << "Non-admin fingerprint (index: " << result->index << ") attempted admin access - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logAdminAccess(result->index, true, false, false);
    m_auditLogger->logUnauthorizedAccess(result->index,
                                         QString("Non-admin user (ID %1) attempted admin access with valid password")
                                             .arg(result->index));
    
    lockDashboardAfterAdminFailure();
    
    emit unauthorizedAccessDetected(QString("WARNING: User ID %1 attempted unauthorized admin access.\n\n"
                                            "This incident has been logged and reported.")
                                        .arg(result->index));
    emit adminAccessDenied("Insufficient privileges");
    return;
  }

  // Check confidence threshold
  if (result->confidence < carbio::security::MIN_ADMIN_CONFIDENCE)
  {
    qWarning() << "Too low confidence (confidence:" << result->confidence << ") - initiating security lockdown";
    
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logUnauthorizedAccess(result->index,
                                         QString("Too low confidence during admin access (confidence: %1)")
                                             .arg(result->confidence));

    lockDashboardAfterAdminFailure();
    emit adminAccessDenied(QString("Too low confidence (%1). Try again.").arg(result->confidence));
    return;
  }

  // Generate session token
  QString token = m_sessionManager->generateToken(result->index);
  if (token.isEmpty())
  {
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_DENIED, result->index,
                            carbio::security::AuthResult::SYSTEM_ERROR, "Failed to generate session token");

    emit adminAccessDenied("System error - token generation failed");
    return;
  }

  // Log successful access
  m_auditLogger->logAdminAccess(result->index, true, true, true);
  m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_GRANTED, result->index,
                          carbio::security::AuthResult::SUCCESS,
                          QString("Admin %1 granted access with confidence %2").arg(result->index).arg(result->confidence));

  setIsProcessing(false);
  m_adminAuthPhase = AdminAuthPhase::COMPLETED;
  setAdminMenuAccessible(true);
  setAdminAccessToken(token);

  qInfo() << "Admin access granted to fingerprint index:" << result->index;
  emit adminAccessGranted(token);
}

void Controller::revokeAdminAccess()
{
  qInfo() << "Admin access revoked";

  m_sessionManager->revokeAllSessions();
  m_adminAuthPhase = AdminAuthPhase::IDLE;
  setAdminMenuAccessible(false);
  setAdminAccessToken("");

  m_auditLogger->logEvent(carbio::security::SecurityEvent::SESSION_ENDED, 0,
                          carbio::security::AuthResult::SUCCESS, "Admin session manually revoked");
  
  emit adminAccessRevoked();
}

void Controller::lockDashboardAfterAdminFailure()
{
  qInfo() << "SECURITY EVENT: Locking dashboard after admin authentication failure";
  
  m_failedAttempts = 0;
  emit failedAttemptsChanged();
  
  setAuthState(AuthState::SCANNING);

  if (!m_scanTimer->isActive())
  {
    qInfo() << "Restarting authentication scanner for re-entry";
    startAuthentication();
  }
  
  qInfo() << "Dashboard locked - user must re-authenticate to regain access";
}

bool Controller::isAdminFingerprint(int fingerprintId) const
{
  return fingerprintId >= carbio::security::ADMIN_ID_MIN && fingerprintId <= carbio::security::ADMIN_ID_MAX;
}

void Controller::setAdminMenuAccessible(bool accessible)
{
  if (m_isAdminMenuAccessible != accessible)
  {
    m_isAdminMenuAccessible = accessible;
    emit isAdminMenuAccessibleChanged();
  }
}

void Controller::setAdminAccessToken(const QString &token)
{
  if (m_adminAccessToken != token)
  {
    m_adminAccessToken = token;
    emit adminAccessTokenChanged();
  }
}

// ============================================================================
// Adaptive Polling Methods
// ============================================================================

void Controller::adjustAuthPollingInterval()
{
  // Only start backoff after threshold to maintain responsiveness
  if (m_consecutiveNoFingerAuth < POLL_BACKOFF_THRESHOLD)
  {
    return;  // Stay at minimum interval for immediate response
  }

  // Exponential backoff: interval = min(current * 2, MAX)
  int currentInterval = m_scanTimer->interval();
  int newInterval = std::min(currentInterval * POLL_BACKOFF_FACTOR, POLL_INTERVAL_MAX);

  if (newInterval != currentInterval)
  {
    m_scanTimer->setInterval(newInterval);
    qDebug() << "Auth polling interval adjusted:" << currentInterval << "ms →" << newInterval << "ms"
             << "(consecutive misses:" << m_consecutiveNoFingerAuth << ")";
  }
}

void Controller::adjustAdminPollingInterval()
{
  // Only start backoff after threshold to maintain responsiveness
  if (m_consecutiveNoFingerAdmin < POLL_BACKOFF_THRESHOLD)
  {
    return;  // Stay at minimum interval for immediate response
  }

  // Exponential backoff: interval = min(current * 2, MAX)
  int currentInterval = m_adminFingerprintTimer->interval();
  int newInterval = std::min(currentInterval * POLL_BACKOFF_FACTOR, POLL_INTERVAL_MAX);

  if (newInterval != currentInterval)
  {
    m_adminFingerprintTimer->setInterval(newInterval);
    qDebug() << "Admin polling interval adjusted:" << currentInterval << "ms →" << newInterval << "ms"
             << "(consecutive misses:" << m_consecutiveNoFingerAdmin << ")";
  }
}

void Controller::resetAuthPollingInterval()
{
  m_consecutiveNoFingerAuth = 0;
  if (m_scanTimer->interval() != POLL_INTERVAL_MIN)
  {
    m_scanTimer->setInterval(POLL_INTERVAL_MIN);
    qDebug() << "Auth polling interval reset to" << POLL_INTERVAL_MIN << "ms (finger detected)";
  }
}

void Controller::resetAdminPollingInterval()
{
  m_consecutiveNoFingerAdmin = 0;
  if (m_adminFingerprintTimer->interval() != POLL_INTERVAL_MIN)
  {
    m_adminFingerprintTimer->setInterval(POLL_INTERVAL_MIN);
    qDebug() << "Admin polling interval reset to" << POLL_INTERVAL_MIN << "ms (finger detected)";
  }
}

// ============================================================================
// Profile Management Methods
// ============================================================================

bool Controller::addDriver(const QString &name, bool isAdmin)
{
  uint16_t assignedId = 0;
  if (m_profileManager->addProfile(name, isAdmin, &assignedId))
  {
    qInfo() << "Added driver profile:" << name << "(ID:" << assignedId << ")";
    refreshTemplateCount();
    return true;
  }
  else
  {
    qWarning() << "Failed to add driver profile:" << name;
    return false;
  }
}

bool Controller::deleteDriver(int id)
{
  if (id < 0 || id > 127)
  {
    qWarning() << "Invalid driver ID:" << id;
    return false;
  }

  if (m_profileManager->deleteProfile(static_cast<uint16_t>(id)))
  {
    qInfo() << "Deleted driver profile ID:" << id;

    // Also delete from sensor database
    if (m_sensorAvailable)
    {
      static_cast<void>(m_sensor->erase_model(static_cast<uint16_t>(id), 1));
    }

    refreshTemplateCount();
    return true;
  }
  else
  {
    qWarning() << "Failed to delete driver profile ID:" << id;
    return false;
  }
}

QString Controller::getDriverName(int id) const
{
  if (id < 0 || id > 127)
  {
    return "Invalid ID";
  }

  return m_profileManager->getDriverName(static_cast<uint16_t>(id));
}

QVariantList Controller::getAllDrivers() const
{
  QVariantList drivers;
  auto profiles = m_profileManager->getAllProfiles();

  for (const auto &profile : profiles)
  {
    QVariantMap driver;
    driver["id"] = profile.id;
    driver["name"] = profile.name;
    driver["isAdmin"] = profile.isAdmin;
    driver["createdAt"] = profile.createdAt.toString(Qt::ISODate);
    drivers.append(driver);
  }

  return drivers;
}
