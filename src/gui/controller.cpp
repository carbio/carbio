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

#include "fingerprint/fingerprint_sensor.h"
#include "controller.h"
#include "sensor_worker.h"
#include "notepad_metadata.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <cstdlib>
#include <cstring>

Controller::Controller(QObject *parent)
    : QObject(parent),
      m_sensor(std::make_unique<carbio::fingerprint_sensor>()),
      m_sensorAvailable(false),
      m_sensorInitializing(false),
      m_authManager(std::make_unique<AuthenticationManager>(this)),
      m_userManager(std::make_unique<UserManager>(&m_metadataStore, this)),
      m_enrollmentManager(std::make_unique<EnrollmentManager>(&m_metadataStore, m_userManager.get(), this)),
      m_adminSession(std::make_unique<AdminSession>(this)),
      m_sensorThread(std::make_unique<QThread>()),
      m_sensorWorker(std::make_unique<SensorWorker>(m_sensor.get())),
      m_isProcessing(false),
      m_templateCount(0),
      m_isAdminMenuAccessible(false),
      m_scanProgress(0),
      m_notepadCache{},  // Zero-initialize all pages
      m_notepadPagesRead(0),
      m_notepadPagesToRead(0) {

  // Move worker to thread
  m_sensorWorker->moveToThread(m_sensorThread.get());

  // NOTE: Do NOT use deleteLater with unique_ptr - it causes double deletion!
  // The unique_ptr will handle cleanup after thread finishes

  // === Authentication Manager Connections ===
  connect(m_authManager.get(), &AuthenticationManager::authStateChanged,
          this, &Controller::authStateChanged);
  connect(m_authManager.get(), &AuthenticationManager::failedAttemptsChanged,
          this, &Controller::failedAttemptsChanged);
  connect(m_authManager.get(), &AuthenticationManager::lockoutSecondsChanged,
          this, &Controller::lockoutSecondsChanged);
  connect(m_authManager.get(), &AuthenticationManager::currentUserChanged,
          this, &Controller::currentUserChanged);
  connect(m_authManager.get(), &AuthenticationManager::authenticationSuccess,
          this, &Controller::authenticationSuccess);
  connect(m_authManager.get(), &AuthenticationManager::authenticationFailed,
          this, &Controller::authenticationFailed);
  connect(m_authManager.get(), &AuthenticationManager::lockoutTriggered,
          this, &Controller::lockoutTriggered);
  connect(m_authManager.get(), &AuthenticationManager::lockoutExpired,
          this, [this]() {
            emit lockoutExpired();  // Forward to QML
            onAuthManagerLockoutExpired();  // Internal handler
          });

  // === Enrollment Manager Connections ===
  connect(m_enrollmentManager.get(), &EnrollmentManager::isEnrollingChanged,
          this, [this]() {
            setIsProcessing(m_enrollmentManager->isEnrolling());
          });
  connect(m_enrollmentManager.get(), &EnrollmentManager::enrollmentStatusChanged,
          this, [this]() {
            setOperationProgress(m_enrollmentManager->enrollmentStatus());
          });
  connect(m_enrollmentManager.get(), &EnrollmentManager::enrollmentRequested,
          this, [this](int userId) {
            qInfo() << "Enrollment requested for user ID:" << userId;
            enableSensorLed();
            QMetaObject::invokeMethod(m_sensorWorker.get(), "enrollFingerprint", Qt::QueuedConnection, Q_ARG(int, userId));
          });
  connect(m_enrollmentManager.get(), &EnrollmentManager::enrollmentSuccess,
          this, &Controller::onEnrollmentSuccess);
  connect(m_enrollmentManager.get(), &EnrollmentManager::enrollmentError,
          this, &Controller::onEnrollmentError);
  connect(m_enrollmentManager.get(), &EnrollmentManager::enrollmentCancelled,
          this, [this]() {
            QMetaObject::invokeMethod(m_sensorWorker.get(), "resetSensorState",
                                      Qt::QueuedConnection);
            disableSensorLed();
            emit operationFailed("Enrollment cancelled by user");
          });

  // === SensorWorker Initialization Connection ===
  connect(m_sensorWorker.get(), &SensorWorker::sensorInitialized,
          this, [this](bool success) {
            setSensorInitializing(false);
            m_sensorAvailable = success;
            emit sensorAvailableChanged();

            if (success) {
              refreshTemplateCount();
              loadMetadataFromNotepad();
              synchronizeMetadata();
            } else {
              qWarning() << "[Controller] Sensor initialization failed";
            }
          });

  // === SensorWorker Authentication Connections ===
  connect(m_sensorWorker.get(), &SensorWorker::authenticationSuccess,
          this, &Controller::onAuthenticationSuccess);
  connect(m_sensorWorker.get(), &SensorWorker::authenticationFailed,
          this, &Controller::onAuthenticationFailed);
  connect(m_sensorWorker.get(), &SensorWorker::authenticationNoFinger,
          this, &Controller::onAuthenticationNoFinger);

  // === SensorWorker Admin Auth Connections ===
  connect(m_sensorWorker.get(), &SensorWorker::adminFingerprintSuccess,
          this, &Controller::onAdminFingerprintSuccess);
  connect(m_sensorWorker.get(), &SensorWorker::adminFingerprintFailed,
          this, &Controller::onAdminFingerprintFailed);
  connect(m_sensorWorker.get(), &SensorWorker::adminFingerprintNoFinger,
          this, &Controller::onAdminFingerprintNoFinger);

  // === SensorWorker Enrollment Connections ===
  connect(m_sensorWorker.get(), &SensorWorker::enrollmentProgress,
          this, &Controller::setOperationProgress);
  connect(m_sensorWorker.get(), &SensorWorker::enrollmentComplete,
          this, &Controller::onEnrollmentComplete);
  connect(m_sensorWorker.get(), &SensorWorker::enrollmentFailed,
          this, &Controller::onEnrollmentFailed);

  // === SensorWorker Operation Connections ===
  connect(m_sensorWorker.get(), &SensorWorker::operationComplete,
          this, &Controller::onOperationComplete);
  connect(m_sensorWorker.get(), &SensorWorker::operationFailed,
          this, &Controller::onOperationFailed);
  connect(m_sensorWorker.get(), &SensorWorker::progressUpdate,
          this, &Controller::setOperationProgress);
  connect(m_sensorWorker.get(), &SensorWorker::templateCountUpdated,
          this, &Controller::setTemplateCount);
  connect(m_sensorWorker.get(), &SensorWorker::scanProgressUpdate,
          this, &Controller::setScanProgress);
  connect(m_sensorWorker.get(), &SensorWorker::enrolledTemplateIds,
          this, &Controller::onEnrolledTemplateIdsReceived);

  // === Notepad Connections ===
  connect(m_sensorWorker.get(), &SensorWorker::notepadDataRead,
          this, &Controller::onNotepadDataRead);
  connect(m_sensorWorker.get(), &SensorWorker::notepadReadFailed,
          this, &Controller::onNotepadReadFailed);
  connect(m_sensorWorker.get(), &SensorWorker::notepadWriteComplete,
          this, &Controller::onNotepadWriteComplete);
  connect(m_sensorWorker.get(), &SensorWorker::notepadWriteFailed,
          this, &Controller::onNotepadWriteFailed);

  // === Admin Session Connections ===
  connect(m_adminSession.get(), &AdminSession::sessionExpired, this, [this]() {
    setAdminMenuAccessible(false);
    emit adminAccessRevoked();
  });

  // === UserManager Connections ===
  connect(m_userManager.get(), &UserManager::userAdded, this, [this](int templateId) {
    auto meta = m_metadataStore.get(static_cast<uint16_t>(templateId));
    if (meta.has_value()) {
      writeMetadataToNotepad(meta.value());
    }
  });

  connect(m_userManager.get(), &UserManager::userUpdated, this, [this](int templateId) {
    auto meta = m_metadataStore.get(static_cast<uint16_t>(templateId));
    if (meta.has_value()) {
      writeMetadataToNotepad(meta.value());
    }
  });

  connect(m_userManager.get(), &UserManager::userRemoved, this, [this](int templateId) {
    static const QByteArray emptyPage(32, 0x00);  // Static - allocated once
    QMetaObject::invokeMethod(m_sensorWorker.get(), "writeNotepad",
                              Qt::QueuedConnection,
                              Q_ARG(int, templateId),
                              Q_ARG(QByteArray, emptyPage));
  });

  // Start worker thread
  m_sensorThread->start();

  // CRITICAL: Initialize timers on worker thread IMMEDIATELY
  // Timers have thread affinity and must be created on the thread where they run!
  QMetaObject::invokeMethod(m_sensorWorker.get(), "initializeTimers", Qt::QueuedConnection);
}

Controller::~Controller() {
  // CRITICAL: Worker lives on worker thread due to moveToThread()
  // Must delete it using deleteLater() and release unique_ptr ownership
  // to prevent double-deletion (once by deleteLater, once by unique_ptr)

  if (m_sensorWorker) {
    // Stop all timers first to prevent new work
    QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAllTimers", Qt::DirectConnection);

    m_sensorWorker->deleteLater();  // Schedule deletion on worker thread
    m_sensorWorker.release();       // Release unique_ptr ownership
  }

  // Stop thread and wait for it to finish (with timeout to prevent hang)
  if (m_sensorThread) {
    m_sensorThread->quit();
    if (!m_sensorThread->wait(2000)) {  // Wait max 2 seconds
      qWarning() << "[Controller] Worker thread did not finish in time, forcing termination";
      m_sensorThread->terminate();
      m_sensorThread->wait(1000);  // Give it 1 more second after terminate
    }
  }
}

// ============================================================================
// Property Getters (Delegate to Managers)
// ============================================================================

int Controller::authState() const {
  return static_cast<int>(m_authManager->authState());
}

int Controller::failedAttempts() const {
  return m_authManager->failedAttempts();
}

int Controller::lockoutSeconds() const {
  return m_authManager->lockoutSeconds();
}

int Controller::currentUserId() const {
  return m_authManager->currentUserId();
}

QString Controller::currentUserName() const {
  int userId = m_authManager->currentUserId();
  if (userId < 0) [[unlikely]] {
    return "";
  }

  auto metadata = m_metadataStore.get(static_cast<uint16_t>(userId));
  if (!metadata.has_value()) [[unlikely]] {
    return QString("User #%1").arg(userId);
  }

  return QString::fromStdString(metadata->display_name);
}

QString Controller::currentUserRoleName() const {
  int userId = m_authManager->currentUserId();
  if (userId < 0) [[unlikely]] {
    return "User";
  }

  auto metadata = m_metadataStore.get(static_cast<uint16_t>(userId));
  if (!metadata.has_value()) [[unlikely]] {
    return "User";
  }

  return m_userManager->getRoleName(static_cast<int>(metadata->role));
}

QString Controller::currentUserRoleColor() const {
  int userId = m_authManager->currentUserId();
  if (userId < 0) [[unlikely]] {
    return "#01E4E0";
  }

  auto metadata = m_metadataStore.get(static_cast<uint16_t>(userId));
  if (!metadata.has_value()) [[unlikely]] {
    return "#01E4E0";
  }

  return m_userManager->getRoleColor(static_cast<int>(metadata->role));
}

// ============================================================================
// Initialization
// ============================================================================

bool Controller::initializeSensor() {
  qInfo() << "[Controller] Starting async sensor initialization on worker thread";

  const char *portEnv = std::getenv("FINGERPRINT_PORT");
  const char *port = portEnv ? portEnv : "/dev/ttyAMA0";

  // Set initializing state immediately (GUI can show loading indicator)
  setSensorInitializing(true);

  // Delegate blocking connect() to worker thread - result comes via sensorInitialized signal
  QMetaObject::invokeMethod(m_sensorWorker.get(), "initializeSensor", Qt::QueuedConnection, Q_ARG(const char*, port));

  // Return true to indicate initialization started (not completed!)
  return true;
}

void Controller::synchronizeMetadata() {
  if (!m_sensorAvailable) [[unlikely]] {
    qWarning() << "[Sync] Cannot synchronize: sensor not available";
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker.get(), "getEnrolledTemplateIds",
                            Qt::QueuedConnection);
}

void Controller::cleanupBeforeExit() {
  if (!m_sensorAvailable) [[unlikely]] return;

  // Stop polling timers
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAuthenticationPolling",
                            Qt::QueuedConnection);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAdminPolling",
                            Qt::QueuedConnection);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAllTimers",
                            Qt::QueuedConnection);

  // Only turn off LED if not currently enrolling
  // (enrolling needs LED for visual feedback!)
  if (!m_isProcessing) {
    QMetaObject::invokeMethod(m_sensorWorker.get(), &SensorWorker::turnLedOff,
                              Qt::QueuedConnection);
  }
}

// ============================================================================
// Authentication Operations (Delegate to AuthenticationManager)
// ============================================================================

void Controller::startAuthentication() {
  if (!ensureSensorAvailable()) [[unlikely]] return;

  if (m_authManager->isLocked()) [[unlikely]] {
    qWarning() << "System locked out";
    return;
  }

  qInfo() << "Starting authentication polling";
  m_authManager->startScanning();
  enableSensorLed();

  QMetaObject::invokeMethod(m_sensorWorker.get(), "startAuthenticationPolling", Qt::QueuedConnection, Q_ARG(int, POLL_INTERVAL_MS));
}

void Controller::resetFailedAttempts() {
  m_authManager->resetFailedAttempts();
}

void Controller::onAuthenticationSuccess(int fingerId, int confidence [[maybe_unused]]) {
  setScanProgress(100);
  m_authManager->recordSuccess(fingerId);

  // Keep LED ON after successful auth (system is now active)
  // LED will turn off only on lockout or system exit
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAuthenticationPolling", Qt::QueuedConnection);

  // Update metadata in-memory only (defer notepad write to avoid blocking)
  auto metadata = m_metadataStore.get(static_cast<uint16_t>(fingerId));
  if (metadata.has_value()) [[likely]] {
    metadata->last_access_at = std::chrono::system_clock::now();
    metadata->successful_auth_count++;
    m_metadataStore.store(*metadata);
    // NOTE: Notepad write deferred - not critical for authentication flow
  }

  QString userName = metadata.has_value()
      ? QString::fromStdString(metadata->display_name)
      : QString("User #%1").arg(fingerId);

  setOperationProgress(QString("Welcome, %1!").arg(userName));
}

void Controller::onAuthenticationFailed() {
  m_authManager->recordFailure();

  // Note: We don't know which user failed, so we can't update failed_auth_count
  // This would require the sensor to return the closest match even on failure
}

void Controller::onAuthenticationNoFinger() {
  setScanProgress(0);
}

void Controller::onAuthManagerLockoutExpired() {
  startAuthentication();
}

// ============================================================================
// Admin Access
// ============================================================================

void Controller::requestAdminAccess() {
  if (m_adminSession->isActive()) [[unlikely]] {
    setAdminMenuAccessible(true);
    emit adminAccessGranted();
    return;
  }

  if (!ensureSensorAvailable()) [[unlikely]] {
    emit adminAccessDenied("Sensor not available");
    return;
  }

  setAdminMenuAccessible(false);
  emit adminFingerprintRequired();
  setIsProcessing(true);

  QMetaObject::invokeMethod(m_sensorWorker.get(), "startAdminPolling", Qt::QueuedConnection, Q_ARG(int, 3));
}

void Controller::revokeAdminAccess() {
  m_adminSession->endSession();
  setAdminMenuAccessible(false);
  emit adminAccessRevoked();
}

bool Controller::isAdminFingerprint(int fingerprintId) const {
  auto metadata = m_metadataStore.get(static_cast<uint16_t>(fingerprintId));
  if (!metadata.has_value()) return false;
  return metadata->is_admin();
}

bool Controller::hasPermission(int permission) const {
  int userId = m_authManager->currentUserId();
  if (userId < 0) return false;

  auto metadata = m_metadataStore.get(static_cast<uint16_t>(userId));
  if (!metadata.has_value()) return false;

  return metadata->can_perform(static_cast<carbio::Permission>(permission));
}

void Controller::onAdminFingerprintSuccess(int fingerId, int confidence) {
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAdminPolling", Qt::QueuedConnection);

  if (!isAdminFingerprint(fingerId)) [[unlikely]] {
    setIsProcessing(false);
    emit unauthorizedAccessDetected(
        QString("WARNING: User ID %1 attempted unauthorized admin access.")
            .arg(fingerId));
    emit adminAccessDenied("Insufficient privileges");
    return;
  }

  if (confidence < carbio::security::MIN_ADMIN_CONFIDENCE) [[unlikely]] {
    setIsProcessing(false);
    emit adminAccessDenied(
        QString("Too low confidence (%1). Try again.").arg(confidence));
    return;
  }

  setIsProcessing(false);
  m_adminSession->startSession(300);
  setAdminMenuAccessible(true);
  emit adminAccessGranted();
}

void Controller::onAdminFingerprintFailed(const QString& reason) {
  QMetaObject::invokeMethod(m_sensorWorker.get(), "stopAdminPolling", Qt::QueuedConnection);
  setIsProcessing(false);
  emit adminAccessDenied(reason);
}

void Controller::onAdminFingerprintNoFinger() {
  // Silent - worker handles polling
}

// ============================================================================
// Enrollment Operations (Delegate to EnrollmentManager)
// ============================================================================

void Controller::enrollFirstBootUser(QString name, int roleType) {
  if (!ensureSensorAvailable()) [[unlikely]] return;

  m_enrollmentManager->startFirstBootEnrollment(name, roleType);
}

void Controller::enrollFingerprint(int id) {
  if (!ensureSensorAvailable() || !ensureNotAuthenticating()) [[unlikely]] return;

  m_enrollmentManager->startEnrollment(id);
}

void Controller::cancelEnrollment() {
  m_enrollmentManager->cancelEnrollment();
}

void Controller::onEnrollmentComplete(const QString& message [[maybe_unused]]) {
  m_enrollmentManager->onEnrollmentComplete(0);  // Will be set by manager

  QMetaObject::invokeMethod(m_sensorWorker.get(), "refreshTemplateCount", Qt::QueuedConnection);
  // Note: Metadata already written to notepad in onEnrollmentSuccess
}

void Controller::onEnrollmentFailed(const QString& error) {
  m_enrollmentManager->onEnrollmentFailed(error);
}

void Controller::onEnrollmentSuccess(int userId [[maybe_unused]], const QString& message) {
  setIsProcessing(false);
  // Keep LED ON - system transitioning to active state or ready for more enrollments
  // LED will turn off only on system exit or idle state

  // Update metadata state to ACTIVE
  // NOTE: Notepad write handled by UserManager::userAdded signal to avoid duplicate writes
  auto metadata = m_metadataStore.get(static_cast<uint16_t>(userId));
  if (metadata.has_value()) {
    metadata->state = carbio::RoleState::ACTIVE;
    metadata->enrolled_at = std::chrono::system_clock::now();
    m_metadataStore.store(*metadata);
  }

  emit operationComplete(message);
}

void Controller::onEnrollmentError(const QString& error) {
  setIsProcessing(false);
  disableSensorLed();
  setScanProgress(0);
  emit operationFailed(error);
}

// ============================================================================
// Sensor Operations
// ============================================================================

void Controller::refreshTemplateCount() {
  if (!ensureSensorAvailable()) [[unlikely]] {
    setTemplateCount(0);
    return;
  }
  QMetaObject::invokeMethod(m_sensorWorker.get(), "refreshTemplateCount", Qt::QueuedConnection);
}

void Controller::identifyFingerprint() {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "identifyFingerprint", Qt::QueuedConnection);
}

void Controller::verifyFingerprint(int id) {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "verifyFingerprint", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::queryTemplate(int id) {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "queryTemplate", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::deleteFingerprint(int id) {
  if (!ensureSensorAvailable() || !ensureNotAuthenticating()) [[unlikely]] return;

  // Notepad supports only IDs 0-15
  if (id < 0 || id > 15) [[unlikely]] {
    emit operationFailed("Invalid ID. Notepad supports only IDs 0-15.");
    return;
  }

  // Remove from metadata store
  m_metadataStore.remove(static_cast<uint16_t>(id));

  // Clear notepad page for this template
  static const QByteArray emptyPage(32, 0x00);  // Static - allocated once
  QMetaObject::invokeMethod(m_sensorWorker.get(), "writeNotepad",
                            Qt::QueuedConnection,
                            Q_ARG(int, id),
                            Q_ARG(QByteArray, emptyPage));

  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "deleteFingerprint", Qt::QueuedConnection, Q_ARG(int, id));
}

void Controller::clearDatabase() {
  if (!ensureSensorAvailable() || !ensureNotAuthenticating()) [[unlikely]] return;

  m_metadataStore.clear();
  clearAllNotepadPages();

  setIsProcessing(true);
  QMetaObject::invokeMethod(m_sensorWorker.get(), "clearDatabase", Qt::QueuedConnection);
}

void Controller::turnLedOn() {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "turnLedOn", Qt::QueuedConnection);
}

void Controller::turnLedOff() {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "turnLedOff", Qt::QueuedConnection);
}

void Controller::setBaudRate(int baudChoice) {
  if (!ensureSensorAvailable()) [[unlikely]] return;

  if (baudChoice < 1 || baudChoice > 12) [[unlikely]] {
    emit operationFailed("Invalid baud rate choice. Must be 1-12.");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker.get(), "setBaudRate", Qt::QueuedConnection, Q_ARG(int, baudChoice));
}

void Controller::setSecurityLevel(int level) {
  if (!ensureSensorAvailable()) [[unlikely]] return;

  if (level < 1 || level > 5) [[unlikely]] {
    emit operationFailed("Invalid security level");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker.get(), "setSecurityLevel", Qt::QueuedConnection, Q_ARG(int, level));
}

void Controller::setPacketSize(int size) {
  if (!ensureSensorAvailable()) [[unlikely]] return;

  if (size < 0 || size > 3) [[unlikely]] {
    emit operationFailed("Invalid packet size");
    return;
  }

  QMetaObject::invokeMethod(m_sensorWorker.get(), "setPacketSize", Qt::QueuedConnection, Q_ARG(int, size));
}

void Controller::softResetSensor() {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "softResetSensor", Qt::QueuedConnection);
}

void Controller::showSystemSettings() {
  if (!ensureSensorAvailable()) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "showSystemSettings", Qt::QueuedConnection);
}

void Controller::onOperationComplete(const QString& message) {
  setIsProcessing(false);
  emit operationComplete(message);
}

void Controller::onOperationFailed(const QString& error) {
  setIsProcessing(false);
  emit operationFailed(error);
}

void Controller::onEnrolledTemplateIdsReceived(QVector<int> sensorIds) {
  qInfo() << "[Sync] Received" << sensorIds.size() << "template IDs from sensor";

  // Get all metadata IDs
  auto allMetadata = m_metadataStore.get_all();
  QSet<int> metadataIds;
  for (const auto& meta : allMetadata) {
    metadataIds.insert(static_cast<int>(meta.template_id));
  }

  QSet<int> sensorIdSet = QSet<int>(sensorIds.begin(), sensorIds.end());

  // Find discrepancies
  QSet<int> orphanedMetadata = metadataIds - sensorIdSet;  // In metadata but not on sensor
  QSet<int> orphanedTemplates = sensorIdSet - metadataIds; // On sensor but not in metadata

  int issuesFound = orphanedMetadata.size() + orphanedTemplates.size();

  if (issuesFound == 0) [[likely]] {
    qInfo() << "[Sync] ✓ Metadata and sensor are in sync";
    return;
  }

  qWarning() << "[Sync] ⚠ Found" << issuesFound << "synchronization issues:";
  qWarning() << "[Sync]   -" << orphanedMetadata.size() << "orphaned metadata entries (no template on sensor)";
  qWarning() << "[Sync]   -" << orphanedTemplates.size() << "orphaned templates (no metadata on disk)";

  // SYNC STRATEGY:
  // Sensor (flash) is the SOURCE OF TRUTH - it persists across SD card changes
  // 1. Remove metadata for templates that don't exist on sensor
  // 2. Create placeholder metadata for templates without metadata

  // Step 1: Remove orphaned metadata
  for (int id : orphanedMetadata) {
    qWarning() << "[Sync] Removing orphaned metadata for ID" << id;
    m_metadataStore.remove(static_cast<uint16_t>(id));
  }

  // Step 2: Create placeholder metadata for orphaned templates
  for (int id : orphanedTemplates) {
    qWarning() << "[Sync] Creating placeholder metadata for template ID" << id;

    carbio::TemplateMetadata meta{};
    meta.template_id = static_cast<uint16_t>(id);
    meta.role = carbio::Role::Passenger;  // Default to lowest privilege
    meta.state = carbio::RoleState::ACTIVE;
    meta.display_name = "Unknown User #" + std::to_string(id);
    meta.enrolled_at = std::chrono::system_clock::now();
    meta.last_access_at = std::nullopt;
    meta.expires_at = std::nullopt;
    meta.successful_auth_count = 0;
    meta.failed_auth_count = 0;
    meta.notes = "Auto-created during sync - template found on sensor without metadata";

    m_metadataStore.store(meta);
  }

  // Save changes to notepad
  qInfo() << "[Sync] Saving synchronized metadata to notepad...";
  for (const auto& meta : m_metadataStore.get_all()) {
    writeMetadataToNotepad(meta);
  }

  // Refresh UI
  m_userManager->refresh();

  qInfo() << "[Sync] ✓ Synchronization complete. Fixed" << issuesFound << "issues.";
  emit operationComplete(QString("Synchronized metadata: removed %1 orphaned entries, created %2 placeholders")
                         .arg(orphanedMetadata.size())
                         .arg(orphanedTemplates.size()));
}

void Controller::clearNotepad() {
  if (!m_sensorAvailable) [[unlikely]] {
    qWarning() << "[Notepad] Cannot clear notepad: sensor not available";
    emit operationFailed("Sensor not available");
    return;
  }

  qInfo() << "[Notepad] Clearing all 16 notepad pages...";
  clearAllNotepadPages();
  qInfo() << "[Notepad] All notepad pages cleared (zeroed)";
  emit operationComplete("Notepad cleared successfully");
}

void Controller::loadMetadataFromNotepad() {
  if (!m_sensorAvailable) [[unlikely]] return;

  for (auto& page : m_notepadCache) {
    page.fill(0);
  }
  m_notepadPagesRead = 0;
  m_notepadPagesToRead = 16;

  readAllNotepadPages();
}

void Controller::onNotepadDataRead(int pageNumber, const QByteArray& data) {
  if (pageNumber >= 0 && pageNumber < 16 && data.size() == 32) [[likely]] {
    std::memcpy(m_notepadCache[static_cast<size_t>(pageNumber)].data(), data.constData(), 32);
  }
  m_notepadPagesRead++;

  // Check if we've read all pages
  if (m_notepadPagesRead >= m_notepadPagesToRead) [[likely]] {
    processNotepadData();
  }
}

void Controller::onNotepadReadFailed(int pageNumber, const QString& error) {
  qWarning() << "[Notepad] Failed to read page" << pageNumber << ":" << error;
  m_notepadPagesRead++;

  // Continue even if some pages fail
  if (m_notepadPagesRead >= m_notepadPagesToRead) [[likely]] {
    processNotepadData();
  }
}

void Controller::onNotepadWriteComplete(int pageNumber [[maybe_unused]]) {
  // Success - silent (avoid log spam)
}

void Controller::onNotepadWriteFailed(int pageNumber, const QString& error) {
  qWarning() << "[Notepad] Failed to write page" << pageNumber << ":" << error;
}

void Controller::processNotepadData() {
  int restoredMetadata = 0;

  for (size_t page = 0; page < 16; ++page) {
    const auto& data = m_notepadCache[page];

    // Check if page is empty (all zeros)
    bool isEmpty = true;
    for (size_t i = 0; i < 32; ++i) {
      if (data[i] != 0) {
        isEmpty = false;
        break;
      }
    }
    if (isEmpty) [[unlikely]] continue;

    // Convert to QByteArray for deserialize (TODO: refactor deserialize to use std::array)
    QByteArray qdata(reinterpret_cast<const char*>(data.data()), 32);
    auto notepadMeta = NotepadMetadata::deserialize(qdata);
    if (!notepadMeta.has_value()) [[unlikely]] {
      continue;
    }

    // Check if metadata already exists for this template ID
    uint16_t templateId = notepadMeta->template_id;
    if (m_metadataStore.get(templateId).has_value()) [[unlikely]] {
      continue;
    }

    // Restore metadata from notepad
    carbio::TemplateMetadata meta = notepadMeta->toTemplateMetadata();
    m_metadataStore.store(meta);
    restoredMetadata++;
  }

  if (restoredMetadata > 0) [[unlikely]] {
    m_userManager->refresh();
    emit operationComplete(QString("Restored %1 users from sensor notepad").arg(restoredMetadata));
  }

  // Clear cache (zero out)
  for (auto& page : m_notepadCache) {
    page.fill(0);
  }
  m_notepadPagesRead = 0;
  m_notepadPagesToRead = 0;

  // CRITICAL: Start authentication if users exist after loading metadata
  // BUT: Don't auto-start if we're in first-boot setup (templateCount will be 0)
  // The setup wizard will call startAuthentication() after first enrollment
  if (m_userManager->count() > 0 && m_templateCount > 0) [[likely]] {
    qInfo() << "[Controller] Metadata loaded, starting authentication for" << m_userManager->count() << "users";
    startAuthentication();
  } else if (m_userManager->count() > 0 && m_templateCount == 0) {
    qInfo() << "[Controller] Metadata loaded but no templates enrolled - waiting for first-boot setup";
  }
}

void Controller::writeMetadataToNotepad(const carbio::TemplateMetadata& meta) {
  int pageNumber = static_cast<int>(meta.template_id);

  if (pageNumber < 0 || pageNumber > 15) [[unlikely]] {
    qWarning() << "[Notepad] Template" << meta.template_id << "out of range (0-15)";
    return;
  }

  NotepadMetadata notepadMeta = NotepadMetadata::fromTemplateMetadata(meta);
  QByteArray data = notepadMeta.serialize();

  QMetaObject::invokeMethod(m_sensorWorker.get(), "writeNotepad",
                            Qt::QueuedConnection,
                            Q_ARG(int, pageNumber),
                            Q_ARG(QByteArray, data));
}

// ============================================================================
// Guard Methods (DRY Principle)
// ============================================================================

bool Controller::ensureSensorAvailable() {
  if (!m_sensorAvailable) [[unlikely]] {
    emit operationFailed("Sensor not available");
    return false;
  }
  return true;
}

bool Controller::ensureNotAuthenticating() {
  if (m_authManager->isScanning() || m_authManager->isAuthenticating()) [[unlikely]] {
    emit operationFailed("Cannot perform operation during authentication");
    return false;
  }
  return true;
}

// ============================================================================
// Property Setters
// ============================================================================

void Controller::setIsProcessing(bool processing) {
  if (m_isProcessing == processing) return;
  m_isProcessing = processing;
  emit isProcessingChanged();
}

void Controller::setTemplateCount(int count) {
  if (m_templateCount == count) return;
  m_templateCount = count;
  emit templateCountChanged();
}

void Controller::setOperationProgress(const QString &progress) {
  if (m_operationProgress == progress) return;
  m_operationProgress = progress;
  emit operationProgressChanged();
}

void Controller::setAdminMenuAccessible(bool accessible) {
  if (m_isAdminMenuAccessible == accessible) return;
  m_isAdminMenuAccessible = accessible;
  emit isAdminMenuAccessibleChanged();
}

void Controller::setScanProgress(int progress) {
  if (m_scanProgress == progress) return;
  m_scanProgress = progress;
  emit scanProgressChanged();
}

void Controller::setSensorInitializing(bool initializing) {
  if (m_sensorInitializing == initializing) return;
  m_sensorInitializing = initializing;
  emit sensorInitializingChanged();
}

// ============================================================================
// LED Control
// ============================================================================

void Controller::enableSensorLed() {
  if (!m_sensor || !m_sensorAvailable) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "turnLedOn", Qt::QueuedConnection);
}

// ============================================================================
// Notepad Helper Methods (DRY - eliminates duplicate loops)
// ============================================================================

void Controller::clearAllNotepadPages() {
  static const QByteArray emptyPage(32, 0x00);  // Static - allocated once
  for (int page = 0; page < 16; ++page) {
    QMetaObject::invokeMethod(m_sensorWorker.get(), "writeNotepad",
                              Qt::QueuedConnection,
                              Q_ARG(int, page),
                              Q_ARG(QByteArray, emptyPage));
  }
}

void Controller::readAllNotepadPages() {
  for (int page = 0; page < 16; ++page) {
    QMetaObject::invokeMethod(m_sensorWorker.get(), "readNotepad",
                              Qt::QueuedConnection,
                              Q_ARG(int, page));
  }
}

// ============================================================================
// LED Control
// ============================================================================

void Controller::disableSensorLed() {
  if (!m_sensor || !m_sensorAvailable) [[unlikely]] return;
  QMetaObject::invokeMethod(m_sensorWorker.get(), "turnLedOff", Qt::QueuedConnection);
}
