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

#include "carbio/fingerprint/fingerprint_sensor.h"

#include "controller.h"
#include "sensor_worker.h"
#include <QDebug>
#include <QThread>

#include <cstdlib>

Controller::Controller(QObject *parent)
    : QObject(parent),
      m_sensor(std::make_unique<carbio::fingerprint::fingerprint_sensor>()),
      m_authState(AuthState::Off), m_failedAttempts(0),
      m_lockoutSeconds(0), m_sensorAvailable(false), m_isProcessing(false),
      m_templateCount(0), m_operationProgress(""),
      m_isAdminMenuAccessible(false), m_scanProgress(0),
      m_lockoutTimer(new QTimer(this)), m_sensorThread(new QThread(this)),
      m_sensorWorker(new SensorWorker(m_sensor.get())) {
  // Move worker to thread
  m_sensorWorker->moveToThread(m_sensorThread);

  // Qt pattern: deleteLater() ensures cleanup happens in correct thread's event
  // loop
  connect(m_sensorThread, &QThread::finished, m_sensorWorker,
          &QObject::deleteLater);

  // Connect authentication signals
  connect(m_sensorWorker, &SensorWorker::authenticationSuccess, this,
          &Controller::onAuthenticationSuccess);
  connect(m_sensorWorker, &SensorWorker::authenticationFailed, this,
          &Controller::onAuthenticationFailed);
  connect(m_sensorWorker, &SensorWorker::authenticationNoFinger, this,
          &Controller::onAuthenticationNoFinger);

  // Connect admin authentication signals
  connect(m_sensorWorker, &SensorWorker::adminFingerprintSuccess, this,
          &Controller::onAdminFingerprintSuccess);
  connect(m_sensorWorker, &SensorWorker::adminFingerprintFailed, this,
          &Controller::onAdminFingerprintFailed);
  connect(m_sensorWorker, &SensorWorker::adminFingerprintNoFinger, this,
          &Controller::onAdminFingerprintNoFinger);

  // Connect enrollment signals
  connect(m_sensorWorker, &SensorWorker::enrollmentProgress, this,
          &Controller::setOperationProgress);
  connect(m_sensorWorker, &SensorWorker::enrollmentComplete, this,
          &Controller::onEnrollmentComplete);
  connect(m_sensorWorker, &SensorWorker::enrollmentFailed, this,
          &Controller::onEnrollmentFailed);

  // Connect operation signals
  connect(m_sensorWorker, &SensorWorker::operationComplete, this,
          &Controller::onOperationComplete);
  connect(m_sensorWorker, &SensorWorker::operationFailed, this,
          &Controller::onOperationFailed);
  connect(m_sensorWorker, &SensorWorker::progressUpdate, this,
          &Controller::setOperationProgress);
  connect(m_sensorWorker, &SensorWorker::templateCountUpdated, this,
          &Controller::setTemplateCount);
  connect(m_sensorWorker, &SensorWorker::scanProgressUpdate, this,
          &Controller::setScanProgress);

  // Start the worker thread
  m_sensorThread->start();

  // Setup lockout countdown timer (still on main thread for UI updates)
  m_lockoutTimer->setInterval(1000);
  connect(m_lockoutTimer, &QTimer::timeout, this, &Controller::onLockoutTick);

  // qDebug() << "Controller initialized with worker thread";
}

Controller::~Controller() {
  m_sensorThread->quit();
  m_sensorThread->wait();
  // qDebug() << "Controller destroyed - worker thread stopped";
}

bool Controller::initializeSensor() {
  try {
    // qDebug() << "Attempting to initialize fingerprint sensor...";

    // Check for environment variable to override serial port
    const char *portEnv = std::getenv("FINGERPRINT_PORT");
    const char *port = portEnv ? portEnv : "/dev/ttyAMA0";

    // qInfo() << "Using serial port:" << port;
    if (m_sensor->open(port)) {
      m_sensorAvailable = true;
      emit sensorAvailableChanged();

      // Pre-warm device settings cache for instant first auth
      QMetaObject::invokeMethod(m_sensorWorker, "prewarmCache",
                                Qt::QueuedConnection);

      refreshTemplateCount();
      // qInfo() << "Starting authentication scanning";
      startAuthentication();
      return true;
    } else {
      // qWarning() << "Failed to initialize sensor - running in demo mode";
      m_sensorAvailable = false;
      emit sensorAvailableChanged();
      return false;
    }
  } catch (std::exception const &e) {
    qCritical() << "Sensor initialization exception:" << e.what();
    m_sensorAvailable = false;
    emit sensorAvailableChanged();
    return false;
  } catch (...) {
    qCritical() << "Unknown exception during sensor initialization";
    m_sensorAvailable = false;
    emit sensorAvailableChanged();
    return false;
  }
}

void Controller::startAuthentication() {
  if (!m_sensorAvailable) {
    // qWarning() << "Sensor not available";
    return;
  }

  if (m_authState == AuthState::Alert) {
    // qWarning() << "System locked out";
    return;
  }

  // qInfo() << "Starting authentication polling";
  setAuthState(AuthState::Scanning);

  enableSensorAutoFingerDetection();

  // Start authentication polling at normal interval (5ms = 200 Hz)
  QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                            Qt::QueuedConnection,
                            Q_ARG(int, POLL_INTERVAL_NORMAL));
}

void Controller::onAuthenticationSuccess(int fingerId, int /* confidence */) {
  setScanProgress(100);

  m_failedAttempts = 0;
  emit failedAttemptsChanged();

  // qInfo() << "Authentication successful - transitioning to authenticated
  // state";
  disableSensorAutoFingerDetection();

  // Stop authentication polling - user is now authenticated
  QMetaObject::invokeMethod(m_sensorWorker, "stopAuthenticationPolling",
                            Qt::QueuedConnection);

  setOperationProgress(QString("Authentication successful! Finger ID: %1").arg(fingerId));
  emit authenticationSuccess();
  setAuthState(AuthState::On);
}

void Controller::onAuthenticationFailed() { handleAuthenticationFailure(); }

void Controller::onAuthenticationNoFinger() {
  // No longer needed - worker thread handles polling silently
  // This signal is only emitted on errors now
  setScanProgress(0);
}

void Controller::onAdminFingerprintSuccess(int fingerId, int confidence) {
  // qInfo() << "Fingerprint matched - index: " << fingerId << ", confidence:"
  // << confidence;

  // Stop worker thread polling
  QMetaObject::invokeMethod(m_sensorWorker, "stopAdminPolling",
                            Qt::QueuedConnection);

  // Check if fingerprint is admin (ID 0-2 by convention)
  if (!isAdminFingerprint(fingerId)) {
    // qWarning() << "Non-admin fingerprint (index: " << fingerId << ")
    // attempted admin access";

    setIsProcessing(false);
    lockDashboardAfterAdminFailure();

    emit unauthorizedAccessDetected(
        QString("WARNING: User ID %1 attempted unauthorized admin access.")
            .arg(fingerId));
    emit adminAccessDenied("Insufficient privileges");
    return;
  }

  // Check confidence threshold
  if (confidence < carbio::security::MIN_ADMIN_CONFIDENCE) {
    // qWarning() << "Too low confidence (confidence:" << confidence << ")";

    setIsProcessing(false);
    lockDashboardAfterAdminFailure();
    emit adminAccessDenied(
        QString("Too low confidence (%1). Try again.").arg(confidence));
    return;
  }

  // Grant admin access
  setIsProcessing(false);
  setAdminMenuAccessible(true);

  // qInfo() << "Admin access granted to fingerprint index:" << fingerId;
  emit adminAccessGranted();
}

void Controller::onAdminFingerprintFailed(QString reason) {
  // qWarning() << "Admin fingerprint verification failed:" << reason;

  // Stop worker thread polling
  QMetaObject::invokeMethod(m_sensorWorker, "stopAdminPolling",
                            Qt::QueuedConnection);

  setIsProcessing(false);
  lockDashboardAfterAdminFailure();

  emit adminAccessDenied(reason);
}

void Controller::onAdminFingerprintNoFinger() {
  // No longer needed - worker thread handles polling silently
}

void Controller::onEnrollmentComplete(QString message) {
  setIsProcessing(false);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::refreshTemplateCount,
                            Qt::QueuedConnection);
  emit operationComplete(message);
}

void Controller::onEnrollmentFailed(QString error) {
  setIsProcessing(false);
  emit operationFailed(error);
}

void Controller::onOperationComplete(QString message) {
  setIsProcessing(false);
  emit operationComplete(message);
}

void Controller::onOperationFailed(QString error) {
  setIsProcessing(false);
  emit operationFailed(error);
}

void Controller::handleAuthenticationFailure() {
  m_failedAttempts++;
  emit failedAttemptsChanged();

  if (m_failedAttempts >= MAX_ATTEMPTS) {
    // qWarning() << "Max authentication attempts reached - initiating lockout";

    // Stop worker thread polling
    QMetaObject::invokeMethod(m_sensorWorker, "stopAuthenticationPolling",
                              Qt::QueuedConnection);

    m_lockoutSeconds = LOCKOUT_DURATION_SEC;
    emit lockoutSecondsChanged();
    setAuthState(AuthState::Alert);
    m_lockoutTimer->start();

    disableSensorAutoFingerDetection();
    QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn,
                              Qt::QueuedConnection);

    emit lockoutTriggered();
  } else {
    // qDebug() << "Authentication failed - activating BURST MODE (1ms polling
    // for instant retry)";
    emit authenticationFailed();
    setAuthState(AuthState::Scanning);

    // ADAPTIVE POLLING STRATEGY:
    // After failure, user is likely retrying immediately with adjusted finger
    // position Phase 1 (BURST): 1ms polling for 2 seconds - catch instant retry
    // attempts Phase 2 (FAST):  3ms polling thereafter - maintain
    // responsiveness
    //
    // CAVEAT: This bypasses multi-phase AFD's gradual escalation
    // Rationale: User already engaged (finger was detected), optimize for retry
    // speed

    // Activate burst mode as user likely retrying immediately
    QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                              Qt::QueuedConnection,
                              Q_ARG(int, POLL_INTERVAL_ULTRA));

    // Drop back to fast polling after 2 seconds
    QTimer::singleShot(2000, this, [this]() {
      if (m_authState == AuthState::Scanning) {
        // qDebug() << "Burst mode timeout - returning to fast polling";
        QMetaObject::invokeMethod(m_sensorWorker, "startAuthenticationPolling",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, POLL_INTERVAL_FAST));
      }
    });
  }
}

// No longer needed - removed

void Controller::setAuthState(AuthState state) {
  if (m_authState != state) {
    m_authState = state;
    if (state != AuthState::Authenticating) {
      // qDebug() << "Auth state changed to:" <<
      // name(state).data();
    }
    emit authStateChanged();
  }
}

void Controller::setIsProcessing(bool processing) {
  if (m_isProcessing != processing) {
    m_isProcessing = processing;
    // qDebug() << "Processing state changed to:" << (processing ? "true" :
    // "false");
    emit isProcessingChanged();
  }
}

void Controller::setTemplateCount(int count) {
  if (m_templateCount != count) {
    m_templateCount = count;
    // qDebug() << "Template count:" << count;
    emit templateCountChanged();
  }
}

void Controller::setOperationProgress(const QString &progress) {
  if (m_operationProgress != progress) {
    m_operationProgress = progress;
    emit operationProgressChanged();
  }
}

void Controller::refreshTemplateCount() {
  if (!m_sensorAvailable) {
    setTemplateCount(0);
    return;
  }

  // Call worker to refresh template count (non-blocking)
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::refreshTemplateCount,
                            Qt::QueuedConnection);
}

void Controller::onLockoutTick() {
  if (m_lockoutSeconds > 0) {
    m_lockoutSeconds--;
    emit lockoutSecondsChanged();

    if (m_lockoutSeconds <= 0) {
      m_lockoutTimer->stop();

      // qInfo() << "Lockout expired - resetting failed attempts and returning
      // to standby";
      m_failedAttempts = 0;
      emit failedAttemptsChanged();

      setAuthState(AuthState::Scanning);
      startAuthentication();
    }
  }
}

void Controller::findFingerprint() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Finding fingerprint";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::findFingerprint,
                            Qt::QueuedConnection);
}

void Controller::identifyFingerprint() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Identifying fingerprint";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::identifyFingerprint,
                            Qt::QueuedConnection);
}

void Controller::verifyFingerprint(int id) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Verifying fingerprint for ID:" << id;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, "verifyFingerprint",
                            Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::queryTemplate(int id) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Querying template for ID:" << id;
  QMetaObject::invokeMethod(m_sensorWorker, "queryTemplate",
                            Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::deleteFingerprint(int id) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::Scanning ||
      m_authState == AuthState::Authenticating) {
    emit operationFailed("Cannot delete while authentication is active");
    return;
  }

  if (id < 1 || id > 127) {
    emit operationFailed("Invalid ID. Must be between 1 and 127.");
    return;
  }

  // qDebug() << "Deleting fingerprint for ID:" << id;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, "deleteFingerprint",
                            Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::clearDatabase() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::Scanning ||
      m_authState == AuthState::Authenticating) {
    emit operationFailed(
        "Cannot clear database while authentication is active");
    return;
  }

  // qDebug() << "Clearing fingerprint database";
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::clearDatabase,
                            Qt::QueuedConnection);
}

void Controller::turnLedOn() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn,
                            Qt::QueuedConnection);
}

void Controller::turnLedOff() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOff,
                            Qt::QueuedConnection);
}

void Controller::setBaudRate(int baudChoice) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // Validate baud choice range
  if (baudChoice < 1 || baudChoice > 12) {
    emit operationFailed("Invalid baud rate choice. Must be 1-12.");
    return;
  }

  // qDebug() << "Setting baud rate:" << baudChoice;

  QMetaObject::invokeMethod(m_sensorWorker, "setBaudRate", Qt::QueuedConnection,
                            Q_ARG(int, baudChoice));
}

void Controller::setSecurityLevel(int level) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  if (level < 1 || level > 5) {
    emit operationFailed("Invalid security level");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, "setSecurityLevel",
                            Qt::QueuedConnection, Q_ARG(int, level));
}

void Controller::setPacketSize(int size) {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  if (size < 0 || size > 3) {
    emit operationFailed("Invalid packet size");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, "setPacketSize",
                            Qt::QueuedConnection, Q_ARG(int, size));
}

void Controller::softResetSensor() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  // qDebug() << "Soft resetting sensor";

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::softResetSensor,
                            Qt::QueuedConnection);
}

void Controller::showSystemSettings() {
  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::showSystemSettings,
                            Qt::QueuedConnection);
}

// ============================================================================
// Admin Authentication Methods
// ============================================================================

void Controller::requestAdminAccess() {
  // qInfo() << "Admin access requested";

  if (!m_sensorAvailable) {
    emit adminAccessDenied("Sensor not available");
    return;
  }

  // Reset state
  setAdminMenuAccessible(false);

  // Direct biometric authentication - no password needed
  emit adminFingerprintRequired();

  // qInfo() << "Starting admin fingerprint scan with fast polling (3ms = 333
  // Hz)";
  setIsProcessing(true);

  // User is actively placing finger - use fast polling (3ms = 333 Hz)
  QMetaObject::invokeMethod(m_sensorWorker, "startAdminPolling",
                            Qt::QueuedConnection, Q_ARG(int, 3));
}

// verifyAdminPassword and startAdminFingerprintScan removed - single-factor
// biometric auth only

void Controller::revokeAdminAccess() {
  // qInfo() << "Admin access revoked";

  setAdminMenuAccessible(false);
  emit adminAccessRevoked();
}

void Controller::lockDashboardAfterAdminFailure() {
  // qInfo() << "locking dashboard after admin authentication failure";

  m_failedAttempts = 0;
  emit failedAttemptsChanged();

  // Restart authentication if not already running
  if (m_authState != AuthState::Scanning) {
    // qInfo() << "restarting authentication scanner for re-entry";
    startAuthentication();
  }

  // qInfo() << "user must re-authenticate to regain access";
}

bool Controller::isAdminFingerprint(int fingerprintId) const {
  return fingerprintId >= carbio::security::ADMIN_ID_MIN &&
         fingerprintId <= carbio::security::ADMIN_ID_MAX;
}

void Controller::setAdminMenuAccessible(bool accessible) {
  if (m_isAdminMenuAccessible != accessible) {
    m_isAdminMenuAccessible = accessible;
    emit isAdminMenuAccessibleChanged();
  }
}

void Controller::setScanProgress(int progress) {
  if (m_scanProgress != progress && progress > 0) // skip 0->0 transitions
  {
    m_scanProgress = progress;
    // qDebug() << "Scan progress:" << progress << "%";
    emit scanProgressChanged();
  }
}

void Controller::enableSensorAutoFingerDetection() {
  if (!m_sensor || !m_sensorAvailable) {
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOn,
                            Qt::QueuedConnection);
  // qDebug() << "AFD enabled (LED ON proxy)";
}

void Controller::disableSensorAutoFingerDetection() {
  if (!m_sensor || !m_sensorAvailable) {
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOff,
                            Qt::QueuedConnection);
  // qDebug() << "AFD disabled (LED OFF proxy)";
}

// ============================================================================
// Profile Management Methods
// ============================================================================

void Controller::enrollFingerprint(int id) {
  // Validate inputs
  if (id < 0 || id > 127) {
    emit operationFailed("Invalid ID. Must be between 0 and 127.");
    return;
  }

  if (!m_sensorAvailable) {
    emit operationFailed("Sensor not available");
    return;
  }

  if (m_authState == AuthState::Scanning ||
      m_authState == AuthState::Authenticating) {
    emit operationFailed(
        "Cannot enroll while authentication is active. Close settings first.");
    return;
  }

  // qInfo() << "Starting fingerprint enrollment for ID:" << id << "(Admin:" <<
  // (id <= 2 ? "Yes" : "No") << ")";

  // Enroll fingerprint directly to sensor
  setIsProcessing(true);
  setOperationProgress("Please scan your finger...");

  QMetaObject::invokeMethod(m_sensorWorker, "enrollFingerprint",
                            Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::cleanupBeforeExit() {
  // Turn LED off before app exit
  // This must complete synchronously to ensure LED is off before process
  // terminates Using BlockingQueuedConnection ensures the LED off command
  // completes before returning

  if (!m_sensorAvailable) {
    return;
  }

  // qInfo() << "Cleaning up sensor before exit - turning LED off";

  // Stop all polling first
  QMetaObject::invokeMethod(m_sensorWorker, "stopAuthenticationPolling",
                            Qt::QueuedConnection);

  // Must use BlockingQueuedConnection to ensure LED turns off before app exits
  // QueuedConnection would queue the command but app might exit before it
  // executes
  QMetaObject::invokeMethod(m_sensorWorker, &SensorWorker::turnLedOff,
                            Qt::BlockingQueuedConnection);

  // qInfo() << "Sensor cleanup complete";
}