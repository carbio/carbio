#include "controller.h"
#include "sensor_worker.h"

#include "carbio/fingerprint_sensor.h"

//#include <QDebug>
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
  , m_isAdminMenuAccessible(false)
  , m_adminAccessToken("")
  , m_scanProgress(0)
  , m_lockoutTimer(new QTimer(this))
  , m_sensorThread(new QThread(this))
  , m_sensorWorker(new SensorWorker(m_sensor.get()))
  , m_adminAuth(std::make_unique<carbio::security::AdminAuthenticator>(this))
  , m_sessionManager(std::make_unique<carbio::security::SessionManager>(this))
  , m_auditLogger(std::make_unique<carbio::security::AuditLogger>(this))
  , m_profileManager(std::make_unique<carbio::security::ProfileManager>(this))
  , m_adminAuthPhase(AdminAuthPhase::IDLE)
{
  // Move worker to thread
  m_sensorWorker->moveToThread(m_sensorThread);

  // Qt pattern: deleteLater() ensures cleanup happens in correct thread's event loop
  connect(m_sensorThread, &QThread::finished, m_sensorWorker, &QObject::deleteLater);

  // Connect authentication signals
  connect(m_sensorWorker, &SensorWorker::authenticationSuccess, this, &Controller::onAuthenticationSuccess);
  connect(m_sensorWorker, &SensorWorker::authenticationFailed, this, &Controller::onAuthenticationFailed);
  connect(m_sensorWorker, &SensorWorker::authenticationNoFinger, this, &Controller::onAuthenticationNoFinger);

  // Connect admin authentication signals
  connect(m_sensorWorker, &SensorWorker::adminFingerprintSuccess, this, &Controller::onAdminFingerprintSuccess);
  connect(m_sensorWorker, &SensorWorker::adminFingerprintFailed, this, &Controller::onAdminFingerprintFailed);
  connect(m_sensorWorker, &SensorWorker::adminFingerprintNoFinger, this, &Controller::onAdminFingerprintNoFinger);

  // Connect enrollment signals
  connect(m_sensorWorker, &SensorWorker::enrollmentProgress, this, &Controller::setOperationProgress);
  connect(m_sensorWorker, &SensorWorker::enrollmentComplete, this, &Controller::onEnrollmentComplete);
  connect(m_sensorWorker, &SensorWorker::enrollmentFailed, this, &Controller::onEnrollmentFailed);

  // Connect operation signals
  connect(m_sensorWorker, &SensorWorker::operationComplete, this, &Controller::onOperationComplete);
  connect(m_sensorWorker, &SensorWorker::operationFailed, this, &Controller::onOperationFailed);
  connect(m_sensorWorker, &SensorWorker::progressUpdate, this, &Controller::setOperationProgress);
  connect(m_sensorWorker, &SensorWorker::templateCountUpdated, this, &Controller::setTemplateCount);
  connect(m_sensorWorker, &SensorWorker::scanProgressUpdate, this, &Controller::setScanProgress);

  // Start the worker thread
  m_sensorThread->start();

  // Setup lockout countdown timer (still on main thread for UI updates)
  m_lockoutTimer->setInterval(1000);
  connect(m_lockoutTimer, &QTimer::timeout, this, &Controller::onLockoutTick);

  // Connect admin auth signals
  connect(m_adminAuth.get(), &carbio::security::AdminAuthenticator::passwordVerified, this, &Controller::adminPasswordVerified);
  connect(m_adminAuth.get(), &carbio::security::AdminAuthenticator::passwordFailed, this, &Controller::adminPasswordFailed);
  connect(m_auditLogger.get(), &carbio::security::AuditLogger::unauthorizedAccessDetected, this, &Controller::unauthorizedAccessDetected);

  // Load user profiles
  if (!m_profileManager->loadProfiles())
  {
    // qWarning() << "Failed to load profiles, starting with empty profile list";
  }

  // qDebug() << "Controller initialized with security components and worker thread";
}

Controller::~Controller()
{
  m_sensorThread->quit();
  m_sensorThread->wait();
  // qDebug() << "Controller destroyed - worker thread stopped";
}

bool Controller::initializeSensor()
{
  try
  {
    // qDebug() << "Attempting to initialize fingerprint sensor...";

    // Check for environment variable to override serial port
    const char* portEnv = std::getenv("FINGERPRINT_PORT");
    const char* port = portEnv ? portEnv : "/dev/ttyAMA0";

    // qInfo() << "Using serial port:" << port;
    if (m_sensor->open(port))
    {
      m_sensorAvailable = true;
      emit sensorAvailableChanged();

      // Pre-warm device settings cache for instant first auth
      QMetaObject::invokeMethod(m_sensorWorker, "prewarmCache", Qt::QueuedConnection);

      refreshTemplateCount();
      // qInfo() << "Starting authentication scanning";
      startAuthentication();
      return true;
    }
    else
    {
      // qWarning() << "Failed to initialize sensor - running in demo mode";
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
    // qWarning() << "Sensor not available";
    return;
  }

  if (m_authState == AuthState::ALERT)
  {
    // qWarning() << "System locked out";
    return;
  }

  // qInfo() << "Starting authentication with FAST polling (3ms = 333 Hz)";
  setAuthState(AuthState::SCANNING);

  enableSensorAutoFingerDetection();

  QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                           Qt::QueuedConnection, Q_ARG(int, POLL_INTERVAL_FAST));
}

void Controller::onAuthenticationSuccess(int fingerId, int /* confidence */, QString /* driverName */)
{
  // Stop worker thread polling
  QMetaObject::invokeMethod(m_sensorWorker, "stopAuthenticationPolling", Qt::QueuedConnection);

  setScanProgress(100);

  m_failedAttempts = 0;
  emit failedAttemptsChanged();

  m_driverName = lookupDriverName(static_cast<uint16_t>(fingerId));
  emit driverNameChanged();

  // qInfo() << "Authentication successful - transitioning to authenticated state";
  disableSensorAutoFingerDetection();

  setOperationProgress("Authentication successful!");
  emit authenticationSuccess(m_driverName);
  setAuthState(AuthState::ON);
}

void Controller::onAuthenticationFailed()
{
  handleAuthenticationFailure();
}

void Controller::onAuthenticationNoFinger()
{
  // No longer needed - worker thread handles polling silently
  // This signal is only emitted on errors now
  setScanProgress(0);
}

void Controller::onAdminFingerprintSuccess(int fingerId, int confidence)
{
  // qInfo() << "Fingerprint matched - index: " << fingerId << ", confidence:" << confidence;

  // Stop worker thread polling
  QMetaObject::invokeMethod(m_sensorWorker, "stopAdminPolling", Qt::QueuedConnection);

  // Check if fingerprint is admin (ID 0-2)
  if (!isAdminFingerprint(fingerId))
  {
    // qWarning() << "Non-admin fingerprint (index: " << fingerId << ") attempted admin access - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logAdminAccess(fingerId, true, false, false);
    m_auditLogger->logUnauthorizedAccess(fingerId,
                                         QString("Non-admin user (ID %1) attempted admin access with valid password")
                                             .arg(fingerId));

    lockDashboardAfterAdminFailure();

    emit unauthorizedAccessDetected(QString("WARNING: User ID %1 attempted unauthorized admin access.\n\n"
                                            "This incident has been logged and reported.")
                                        .arg(fingerId));
    emit adminAccessDenied("Insufficient privileges");
    return;
  }

  // Check confidence threshold
  if (confidence < carbio::security::MIN_ADMIN_CONFIDENCE)
  {
    // qWarning() << "Too low confidence (confidence:" << confidence << ") - initiating security lockdown";

    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logUnauthorizedAccess(fingerId,
                                         QString("Too low confidence during admin access (confidence: %1)")
                                             .arg(confidence));

    lockDashboardAfterAdminFailure();
    emit adminAccessDenied(QString("Too low confidence (%1). Try again.").arg(confidence));
    return;
  }

  // Generate session token
  QString token = m_sessionManager->generateToken(fingerId);
  if (token.isEmpty())
  {
    setIsProcessing(false);
    m_adminAuthPhase = AdminAuthPhase::IDLE;

    m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_DENIED, fingerId,
                            carbio::security::AuthResult::SYSTEM_ERROR, "Failed to generate session token");

    emit adminAccessDenied("System error - token generation failed");
    return;
  }

  // Log successful access
  m_auditLogger->logAdminAccess(fingerId, true, true, true);
  m_auditLogger->logEvent(carbio::security::SecurityEvent::ADMIN_ACCESS_GRANTED, fingerId,
                          carbio::security::AuthResult::SUCCESS,
                          QString("Admin %1 granted access with confidence %2").arg(fingerId).arg(confidence));

  setIsProcessing(false);
  m_adminAuthPhase = AdminAuthPhase::COMPLETED;
  setAdminMenuAccessible(true);
  setAdminAccessToken(token);

  // qInfo() << "Admin access granted to fingerprint index:" << fingerId;
  emit adminAccessGranted(token);
}

void Controller::onAdminFingerprintFailed(QString reason)
{
  // qWarning() << "Admin fingerprint verification failed:" << reason;

  // Stop worker thread polling
  QMetaObject::invokeMethod(m_sensorWorker, "stopAdminPolling", Qt::QueuedConnection);

  setIsProcessing(false);
  m_adminAuthPhase = AdminAuthPhase::IDLE;

  m_auditLogger->logUnauthorizedAccess(0, reason);
  lockDashboardAfterAdminFailure();

  emit adminAccessDenied(reason);
}

void Controller::onAdminFingerprintNoFinger()
{
  // No longer needed - worker thread handles polling silently
}

void Controller::onEnrollmentComplete(QString message)
{
  setIsProcessing(false);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::refreshTemplateCount, Qt::QueuedConnection);
  emit operationComplete(message);
}

void Controller::onEnrollmentFailed(QString error)
{
  setIsProcessing(false);
  emit operationFailed(error);
}

void Controller::onOperationComplete(QString message)
{
  setIsProcessing(false);
  emit operationComplete(message);
}

void Controller::onOperationFailed(QString error)
{
  setIsProcessing(false);
  emit operationFailed(error);
}

void Controller::handleAuthenticationFailure()
{
  m_failedAttempts++;
  emit failedAttemptsChanged();

  if (m_failedAttempts >= MAX_ATTEMPTS)
  {
    // qWarning() << "Max authentication attempts reached - initiating lockout";

    // Stop worker thread polling
    QMetaObject::invokeMethod(m_sensorWorker, "stopAuthenticationPolling", Qt::QueuedConnection);

    m_lockoutSeconds = LOCKOUT_DURATION_SEC;
    emit lockoutSecondsChanged();
    setAuthState(AuthState::ALERT);
    m_lockoutTimer->start();

    disableSensorAutoFingerDetection();
    QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn, Qt::QueuedConnection);

    emit lockoutTriggered();
  }
  else
  {
    // qDebug() << "Authentication failed - activating BURST MODE (1ms polling for instant retry)";
    emit authenticationFailed();
    setAuthState(AuthState::SCANNING);

    // Activate burst mode as user likely retrying immediately
    QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                             Qt::QueuedConnection, Q_ARG(int, POLL_INTERVAL_ULTRA));

    // Drop back to fast polling after 2 seconds
    QTimer::singleShot(2000, this, [this]() {
      if (m_authState == AuthState::SCANNING) {
        // qDebug() << "Burst mode timeout - returning to fast polling";
        QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                                 Qt::QueuedConnection, Q_ARG(int, POLL_INTERVAL_FAST));
      }
    });
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
    m_authState = state;
    if (state != AuthState::AUTHENTICATING)
    {
      // qDebug() << "Auth state changed to:" << authStateToString(state).data();
    }
    emit authStateChanged();
  }
}

void Controller::setIsProcessing(bool processing)
{
  if (m_isProcessing != processing)
  {
    m_isProcessing = processing;
    // qDebug() << "Processing state changed to:" << (processing ? "true" : "false");
    emit isProcessingChanged();
  }
}

void Controller::setTemplateCount(int count)
{
  if (m_templateCount != count)
  {
    m_templateCount = count;
    // qDebug() << "Template count:" << count;
    emit templateCountChanged();
  }
}

void Controller::setOperationProgress(const QString &progress)
{
  if (m_operationProgress != progress)
  {
    m_operationProgress = progress;
    emit operationProgressChanged();
  }
}

void Controller::refreshTemplateCount()
{
  if (!m_sensorAvailable)
  {
    setTemplateCount(0);
    return;
  }

  // Call worker to refresh template count (non-blocking)
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::refreshTemplateCount, Qt::QueuedConnection);
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

      // qInfo() << "Lockout expired - resetting failed attempts and returning to standby";
      m_failedAttempts = 0;
      emit failedAttemptsChanged();

      setAuthState(AuthState::SCANNING);
      startAuthentication();
    }
  }
}

void Controller::findFingerprint()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Finding fingerprint";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::findFingerprint, Qt::QueuedConnection);
}

void Controller::identifyFingerprint()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Identifying fingerprint";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::identifyFingerprint, Qt::QueuedConnection);
}

void Controller::verifyFingerprint(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Verifying fingerprint for ID:" << id;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, "verifyFingerprint", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::queryTemplate(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Querying template for ID:" << id;
  QMetaObject::invokeMethod(m_sensorWorker, "queryTemplate", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::deleteFingerprint(int id)
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::SCANNING || m_authState == AuthState::AUTHENTICATING)
  {
    emit operationFailed("Cannot delete while authentication is active");
    return;
  }

  if (id < 1 || id > 127)
  {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  // qDebug() << "Deleting fingerprint for ID:" << id;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, "deleteFingerprint", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::clearDatabase()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::SCANNING || m_authState == AuthState::AUTHENTICATING)
  {
    emit operationFailed("Cannot clear database while authentication is active");
    return;
  }

  // qDebug() << "Clearing fingerprint database";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::clearDatabase, Qt::QueuedConnection);
}

void Controller::turnLedOn()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn, Qt::QueuedConnection);
}

void Controller::turnLedOff()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOff, Qt::QueuedConnection);
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

  // qDebug() << "Setting baud rate:" << baudChoice;

  QMetaObject::invokeMethod(m_sensorWorker, "setBaudRate", Qt::QueuedConnection, Q_ARG(int, baudChoice));
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

  QMetaObject::invokeMethod(m_sensorWorker, "setSecurityLevel", Qt::QueuedConnection, Q_ARG(int, level));
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

  QMetaObject::invokeMethod(m_sensorWorker, "setPacketSize", Qt::QueuedConnection, Q_ARG(int, size));
}

void Controller::softResetSensor()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Soft resetting sensor";

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::softResetSensor, Qt::QueuedConnection);
}

void Controller::showSystemSettings()
{
  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::showSystemSettings, Qt::QueuedConnection);
}

// ============================================================================
// Admin Authentication Methods
// ============================================================================

void Controller::requestAdminAccess()
{
  // qInfo() << "Admin access requested";

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
    // qWarning() << "Password verification called in wrong phase";
    return;
  }

  // qInfo() << "Verifying admin password";

  bool valid = m_adminAuth->verifyPassword(password);
  if (valid)
  {
    // qInfo() << "Password verified";
    m_auditLogger->logEvent(carbio::security::SecurityEvent::PASSWORD_VERIFIED, 0, carbio::security::AuthResult::SUCCESS, "Admin password correct");
    m_adminAuthPhase = AdminAuthPhase::FINGERPRINT_PENDING;
    emit adminFingerprintRequired();
  }
  else
  {
    // qWarning() << "Password verification failed";
    
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
    // qWarning() << "Fingerprint scan called in wrong phase";
    emit adminAccessDenied("Invalid authentication flow");
    return;
  }

  if (!m_sensorAvailable)
  {
    emit adminAccessDenied("Sensor not available");
    return;
  }

  // qInfo() << "Starting admin fingerprint scan with FAST polling (3ms = 333 Hz)";
  setIsProcessing(true);

  // user is actively placing finger hence perform fast polling
  QMetaObject::invokeMethod(m_sensorWorker, "startAdminPolling",
                           Qt::QueuedConnection, Q_ARG(int, POLL_INTERVAL_FAST));
}

void Controller::revokeAdminAccess()
{
  // qInfo() << "Admin access revoked";

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
  // qInfo() << "locking dashboard after admin authentication failure";

  m_failedAttempts = 0;
  emit failedAttemptsChanged();

  // Restart authentication if not already running
  if (m_authState != AuthState::SCANNING)
  {
    // qInfo() << "restarting authentication scanner for re-entry";
    startAuthentication();
  }

  // qInfo() << "user must re-authenticate to regain access";
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

void Controller::setScanProgress(int progress)
{
  if (m_scanProgress != progress && progress > 0) // skip 0->0 transitions
  {
    m_scanProgress = progress;
    // qDebug() << "Scan progress:" << progress << "%";
    emit scanProgressChanged();
  }
}

void Controller::enableSensorAutoFingerDetection()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn, Qt::QueuedConnection);
  // qDebug() << "AFD enabled (LED ON proxy)";
}

void Controller::disableSensorAutoFingerDetection()
{
  if (!m_sensor || !m_sensorAvailable)
  {
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOff, Qt::QueuedConnection);
  // qDebug() << "AFD disabled (LED OFF proxy)";
}

// ============================================================================
// Profile Management Methods
// ============================================================================

void Controller::enrollDriverWithFingerprint(const QString &name, int id, bool isAdmin)
{
  // Validate inputs
  if (name.trimmed().isEmpty())
  {
    emit operationFailed("Driver name cannot be empty");
    return;
  }

  if (id < 1 || id > 127)
  {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  if (!m_sensorAvailable)
  {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::SCANNING || m_authState == AuthState::AUTHENTICATING)
  {
    emit operationFailed("Cannot enroll while authentication is active. Close settings first.");
    return;
  }

  // Check if profile already exists
  if (m_profileManager->profileExists(static_cast<uint16_t>(id)))
  {
    emit operationFailed(QString("Profile with ID %1 already exists").arg(id));
    return;
  }

  // Create profile
  if (!m_profileManager->addProfileWithId(name.trimmed(), static_cast<uint16_t>(id), isAdmin))
  {
    emit operationFailed("Failed to create driver profile");
    return;
  }

  // qInfo() << "Created profile for" << name << "(ID:" << id << ", Admin:" << isAdmin << ")";
  // qInfo() << "Starting fingerprint enrollment...";

  // Enroll fingerprint
  setIsProcessing(true);
  setOperationProgress("Profile created. Please scan your finger...");

  //perform enrollment
  QMetaObject::invokeMethod(m_sensorWorker, "enrollFingerprint", Qt::QueuedConnection, Q_ARG(int, id));
}