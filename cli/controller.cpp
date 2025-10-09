#include "controller.h"

#include <QDebug>
#include <QThread>
#include <cstdlib>

Controller::Controller(QObject *parent)
  : QObject(parent)
  , m_sensor(nullptr)
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

  qDebug() << "Controller initialized with security components";
}

Controller::~Controller()
{
  if (m_sensor)
  {
    m_sensor->end();
  }
}

bool Controller::initializeSensor()
{
  try
  {
    qDebug() << "Attempting to initialize fingerprint sensor...";

    // Check for environment variable to override serial port
    const char* portEnv = std::getenv("FINGERPRINT_PORT");
    const char* port = portEnv ? portEnv : "/dev/ttyAMA0";

    qInfo() << "Using serial port:" << port;
    m_sensor = std::make_unique<carbio::FingerprintSensor>(port);
    if (nullptr != m_sensor && m_sensor->begin())
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
  if (!m_sensorAvailable || !m_sensor)
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
  if (m_sensor->turnLedOn() != carbio::StatusCode::Success)
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

  if (m_sensor && !m_manualLedControl)
  {
    if (m_sensor->turnLedOff() != carbio::StatusCode::Success)
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
    if (m_sensor->turnLedOn() != carbio::StatusCode::Success)
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
  if (!m_sensor || m_authState != AuthState::SCANNING)
  {
    return;
  }

  // Step 1: Check if finger is present (non-blocking)
  carbio::StatusCode status = m_sensor->captureImage();

  // If no finger, implement adaptive polling with exponential backoff
  if (status == carbio::StatusCode::NoFinger)
  {
    m_consecutiveNoFingerAuth++;

    // Adjust polling interval exponentially to save CPU/power
    adjustAuthPollingInterval();

    return;
  }

  // Finger detected! Reset adaptive polling and start authentication
  resetAuthPollingInterval();

  // Stop polling and start authentication
  // Use QTimer::singleShot to prevent race condition
  if (m_scanTimer->isActive())
  {
    m_scanTimer->stop();
  }

  setIsProcessing(true);
  setAuthState(AuthState::AUTHENTICATING);

  if (status != carbio::StatusCode::Success)
  {
    qWarning() << "Failed to capture image:" << static_cast<int>(status);
    handleAuthenticationFailure();
    return;
  }

  qInfo() << "Image captured successfully";

  // Step 2: Convert image to template
  status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
  {
    qWarning() << "Failed to create template:" << static_cast<int>(status);
    handleAuthenticationFailure();
    return;
  }

  qInfo() << "Template created successfully";

  // Step 3: Search for match
  auto result = m_sensor->fastSearch();
  if (!result)
  {
    qWarning() << "Fingerprint not found in database";
    handleAuthenticationFailure();
    return;
  }

  // Success!
  qInfo() << "Authentication successful. Finger ID:" << result->fingerId << "Confidence:" << result->confidence;

  m_scanTimer->stop();
  m_failedAttempts = 0;
  emit failedAttemptsChanged();

  m_driverName = lookupDriverName(result->fingerId);
  emit driverNameChanged();

  // Turn off LED on success (dashboard is now visible)
  if (m_sensor->turnLedOff() != carbio::StatusCode::Success)
  {
    qWarning() << "Failed to turn off LED";
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
    if (m_sensor->turnLedOn() != carbio::StatusCode::Success)
    {
      qWarning() << "Failed to turn on LED";
    }

    emit lockoutTriggered();
  }
  else
  {
    emit authenticationFailed();

    // Brief LED blink to indicate failure, then return to scanning mode
    if (m_sensor->turnLedOff() != carbio::StatusCode::Success)
    {
      qWarning() << "Failed to turn off LED";
    }
    QThread::msleep(200);
    if (m_sensor->turnLedOn() != carbio::StatusCode::Success)
    {
      qWarning() << "Failed to turn on LED";
    }

    // Return to scanning mode and restart polling with reset interval
    setAuthState(AuthState::SCANNING);
    resetAuthPollingInterval();
    m_scanTimer->start();
  }
}

QString Controller::lookupDriverName(uint16_t fingerId)
{
  // In production, this would query a database
  // For now, simple mapping
  switch (fingerId)
  {
  case 0:
    return "Sarah";
  case 1:
    return "Peter";
  case 2:
    return "Maria";
  default:
    return "Driver " + QString::number(fingerId);
  }
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
  static_cast<void>(m_sensor->captureImage()); // Clear image buffer
}

void Controller::refreshTemplateCount()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    setTemplateCount(0);
    return;
  }

  auto templates = m_sensor->fetchTemplates();
  if (templates)
  {
    setTemplateCount(static_cast<int>(templates->size()));
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
        if (m_sensor->turnLedOff() != carbio::StatusCode::Success)
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
  if (!m_sensor || !m_sensorAvailable)
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
  setOperationProgress("Place finger on sensor...");

  carbio::StatusCode status = m_sensor->captureImage();
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to capture first image");
    return;
  }

  status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to create first template");
    return;
  }

  setOperationProgress("Remove finger...");
  QThread::msleep(500);

  // Step 2: Capture second image
  setOperationProgress("Place same finger again...");

  status = m_sensor->captureImage();
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to capture second image");
    return;
  }

  status = m_sensor->imageToTemplate(2);
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to create second template");
    return;
  }

  // Step 3: Create model from both templates
  setOperationProgress("Creating fingerprint model...");
  status = m_sensor->createModel();
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to create fingerprint model");
    return;
  }

  // Step 4: Store in database
  setOperationProgress("Storing template...");
  status = m_sensor->storeTemplate(static_cast<uint16_t>(id));
  if (status != carbio::StatusCode::Success)
  {
    resetSensorState();
    setIsProcessing(false);
    emit operationFailed("Failed to store template");
    return;
  }

  setIsProcessing(false);
  refreshTemplateCount();
  emit operationComplete("Fingerprint enrolled successfully as ID #" + QString::number(id));
}

void Controller::findFingerprint()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Finding fingerprint";
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  carbio::StatusCode status = m_sensor->captureImage();
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  auto result = m_sensor->search(1, 0, 0);
  if (!result)
  {
    setIsProcessing(false);
    emit operationFailed("Fingerprint not found in database");
    return;
  }

  setIsProcessing(false);
  emit operationComplete("Found: ID #" + QString::number(result->fingerId) +
                        " (Confidence: " + QString::number(result->confidence) + ")");
}

void Controller::identifyFingerprint()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Identifying fingerprint";
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  carbio::StatusCode status = m_sensor->captureImage();
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  auto result = m_sensor->fastSearch();
  if (!result)
  {
    setIsProcessing(false);
    emit operationFailed("No matching fingerprint found");
    return;
  }

  QString driverName = lookupDriverName(result->fingerId);
  setIsProcessing(false);
  emit operationComplete("Identified: " + driverName + " [ID #" + QString::number(result->fingerId) +
                        ", Confidence: " + QString::number(result->confidence) + "]");
}

void Controller::verifyFingerprint(int id)
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Verifying fingerprint for ID:" << id;
  setIsProcessing(true);

  emit operationComplete("Place finger on sensor...");

  // Capture image and create template
  carbio::StatusCode status = m_sensor->captureImage();
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to capture image");
    return;
  }

  status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Failed to create template");
    return;
  }

  // Load stored template into buffer 2
  status = m_sensor->loadTemplate(static_cast<uint16_t>(id), 2);
  if (status != carbio::StatusCode::Success)
  {
    setIsProcessing(false);
    emit operationFailed("Template #" + QString::number(id) + " not found");
    return;
  }

  // Search only for this specific ID
  auto result = m_sensor->search(1, static_cast<uint16_t>(id), 1);
  if (!result || result->fingerId != static_cast<uint16_t>(id))
  {
    setIsProcessing(false);
    emit operationFailed("Fingerprint does NOT match ID #" + QString::number(id));
    return;
  }

  setIsProcessing(false);
  emit operationComplete("VERIFIED - ID #" + QString::number(id) +
                        " (Confidence: " + QString::number(result->confidence) + ")");
}

void Controller::queryTemplate(int id)
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Querying template for ID:" << id;

  auto templates = m_sensor->fetchTemplates();
  if (!templates)
  {
    emit operationFailed("Failed to fetch templates");
    return;
  }

  bool found = std::find(templates->begin(), templates->end(), static_cast<uint16_t>(id)) != templates->end();
  if (found)
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
  if (!m_sensor || !m_sensorAvailable)
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

  if (m_sensor->deleteTemplate(static_cast<uint16_t>(id), 1) == carbio::StatusCode::Success)
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
  if (!m_sensor || !m_sensorAvailable)
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

  if (m_sensor->clearDatabase() == carbio::StatusCode::Success)
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
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  m_manualLedControl = true;
  if (m_sensor->turnLedOn() == carbio::StatusCode::Success)
  {
    emit operationComplete("LED turned ON (manual control)");
  }
  else
  {
    m_manualLedControl = false;
    emit operationFailed("Failed to turn LED on");
  }
}

void Controller::turnLedOff()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  m_manualLedControl = true;
  if (m_sensor->turnLedOff() == carbio::StatusCode::Success)
  {
    emit operationComplete("LED turned OFF (manual control)");
  }
  else
  {
    m_manualLedControl = false;
    emit operationFailed("Failed to turn LED off");
  }
}

void Controller::toggleLed()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // Toggle by trying to turn off first, if that fails assume it's off and turn on
  carbio::StatusCode status = m_sensor->turnLedOff();
  if (status == carbio::StatusCode::Success)
  {
    emit operationComplete("LED turned OFF");
  }
  else
  {
    // LED was probably already off, turn it on
    status = m_sensor->turnLedOn();
    if (status == carbio::StatusCode::Success)
    {
      emit operationComplete("LED turned ON");
    }
    else
    {
      emit operationFailed("Failed to toggle LED");
    }
  }
}

void Controller::setBaudRate(int baudChoice)
{
  if (!m_sensor || !m_sensorAvailable)
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

  carbio::BaudRateRegister baud;
  switch (baudChoice)
  {
    case 1: baud = carbio::BaudRateRegister::_9600; break;
    case 2: baud = carbio::BaudRateRegister::_19200; break;
    case 3: baud = carbio::BaudRateRegister::_28800; break;
    case 4: baud = carbio::BaudRateRegister::_38400; break;
    case 5: baud = carbio::BaudRateRegister::_48000; break;
    case 6: baud = carbio::BaudRateRegister::_57600; break;
    case 7: baud = carbio::BaudRateRegister::_67200; break;
    case 8: baud = carbio::BaudRateRegister::_76800; break;
    case 9: baud = carbio::BaudRateRegister::_86400; break;
    case 10: baud = carbio::BaudRateRegister::_96000; break;
    case 11: baud = carbio::BaudRateRegister::_105600; break;
    case 12: baud = carbio::BaudRateRegister::_115200; break;
    default:
      emit operationFailed("Invalid baud rate choice");
      return;
  }

  if (m_sensor->setBaudRate(baud) == carbio::StatusCode::Success)
  {
    emit operationComplete("Baud rate updated. Reconnect required.");
  }
  else
  {
    emit operationFailed("Failed to set baud rate");
  }
}

void Controller::setSecurityLevel(int level)
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (level < 1 || level > 5)
  {
    emit operationFailed("Invalid security level");
    return;
  }

  auto secLevel = static_cast<carbio::SecurityLevelRegister>(level);
  if (m_sensor->setSecurityLevelRegister(secLevel) == carbio::StatusCode::Success)
  {
    emit operationComplete("Security level updated");
  }
  else
  {
    emit operationFailed("Failed to set security level");
  }
}

void Controller::setPacketSize(int size)
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (size < 0 || size > 3)
  {
    emit operationFailed("Invalid packet size");
    return;
  }

  auto packetSize = static_cast<carbio::DataPacketSizeRegister>(size);
  if (m_sensor->setDataPacketSizeRegister(packetSize) == carbio::StatusCode::Success)
  {
    emit operationComplete("Data packet size updated");
  }
  else
  {
    emit operationFailed("Failed to set packet size");
  }
}

void Controller::softResetSensor()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  qDebug() << "Soft resetting sensor";

  if (m_sensor->softReset() == carbio::StatusCode::Success)
  {
    emit operationComplete("Sensor reset successfully");
  }
  else
  {
    emit operationFailed("Failed to reset sensor");
  }
}

void Controller::showSystemSettings()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  auto params = m_sensor->getParameters();
  if (!params)
  {
    emit operationFailed("Failed to read system parameters");
    return;
  }

  QString settings;
  settings += "System Settings:\n";
  settings += "Library Size: " + QString::number(params->capacity) + "\n";
  settings += "Security Level: " + QString::number(params->SecurityLevel) + "\n";
  settings += "Packet Length: " + QString::number(params->packetLength) + "\n";
  settings += "Baud Rate: " + QString::number(params->baudRate);

  emit operationComplete(settings);
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

  if (!m_sensor || !m_sensorAvailable)
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

  if (!m_sensor || !m_sensorAvailable)
  {
    m_adminFingerprintTimer->stop();
    setIsProcessing(false);
    emit adminAccessDenied("Sensor not available");
    return;
  }

  // Check if finger is present (non-blocking poll)
  carbio::StatusCode status = m_sensor->captureImage();

  // If no finger, implement adaptive polling with exponential backoff
  if (status == carbio::StatusCode::NoFinger)
  {
    m_consecutiveNoFingerAdmin++;
    adjustAdminPollingInterval();

    return;
  }

  // Finger detected! Reset adaptive polling and stop polling
  resetAdminPollingInterval();
  m_adminFingerprintTimer->stop();

  // Check if image capture was successful
  if (status != carbio::StatusCode::Success)
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
  carbio::StatusCode status = m_sensor->imageToTemplate(1);
  if (status != carbio::StatusCode::Success)
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
  auto result = m_sensor->fastSearch();
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

  uint16_t fingerprintId = result->fingerId;
  uint16_t confidence = result->confidence;

  qInfo() << "Fingerprint matched - ID:" << fingerprintId << "Confidence:" << confidence;

  // Check if fingerprint is admin (ID 0-2)
  if (!isAdminFingerprint(fingerprintId))
  {
    qWarning() << "Non-admin fingerprint (ID" << fingerprintId << ") attempted admin access - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logAdminAccess(fingerprintId, true, false, false);
    m_auditLogger->logUnauthorizedAccess(fingerprintId,
                                         QString("Non-admin user (ID %1) attempted admin access with valid password")
                                             .arg(fingerprintId));
    
    lockDashboardAfterAdminFailure();
    
    emit unauthorizedAccessDetected(QString("WARNING: User ID %1 attempted unauthorized admin access.\n\n"
                                            "This incident has been logged and reported.")
                                        .arg(fingerprintId));
    emit adminAccessDenied("Insufficient privileges");
    return;
  }

  // Check confidence threshold
  if (confidence < carbio::security::MIN_ADMIN_CONFIDENCE)
  {
    qWarning() << "Too low confidence (confidence:" << confidence << ") - initiating security lockdown";
    
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logUnauthorizedAccess(fingerprintId,
                                         QString("Too low confidence during admin access (confidence: %1)")
                                             .arg(confidence));

    lockDashboardAfterAdminFailure();
    emit adminAccessDenied(QString("Too low confidence (%1). Try again.").arg(confidence));
    return;
  }

  // SUCCESS! Generate session token
  QString token = m_sessionManager->generateToken(fingerprintId);
  if (token.isEmpty())
  {
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_DENIED, fingerprintId,
                            carbio::security::AuthResult::SYSTEM_ERROR, "Failed to generate session token");

    emit adminAccessDenied("System error - token generation failed");
    return;
  }

  // Log successful access
  m_auditLogger->logAdminAccess(fingerprintId, true, true, true);
  m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_GRANTED, fingerprintId,
                          carbio::security::AuthResult::SUCCESS,
                          QString("Admin %1 granted access with confidence %2").arg(fingerprintId).arg(confidence));

  setIsProcessing(false);
  m_adminAuthPhase = AdminAuthPhase::COMPLETED;
  setAdminMenuAccessible(true);
  setAdminAccessToken(token);

  qInfo() << "Admin access granted to fingerprint ID:" << fingerprintId;
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
