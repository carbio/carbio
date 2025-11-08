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

#pragma once

#include "admin_session.h"
#include "authentication_manager.h"
#include "enrollment_manager.h"
#include "template_metadata.h"
#include "user_manager.h"
#include <QObject>
#include <QString>

#include <array>
#include <memory>

// Security constants
namespace carbio::security
{
constexpr uint16_t MIN_ADMIN_CONFIDENCE = 150;
}

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpadded"
#  pragma GCC diagnostic ignored "-Wsuggest-final-types"
#  pragma GCC diagnostic ignored "-Wsuggest-final-methods"
#endif

namespace carbio
{
class fingerprint_sensor;
}

class SensorWorker;
class QThread;

/**
 * @brief Main controller - coordinates between QML, managers, and sensor
 *
 * REFACTORED: Now follows Single Responsibility Principle
 * - Authentication logic → AuthenticationManager
 * - Enrollment logic → EnrollmentManager
 * - Persistence logic → Sensor notepad (flash memory)
 * - Controller is now a thin coordinator/facade
 */
class Controller : public QObject
{
  Q_OBJECT

  // QML Properties (unchanged for backward compatibility)
  Q_PROPERTY(int authState READ authState NOTIFY authStateChanged)
  Q_PROPERTY(int failedAttempts READ failedAttempts NOTIFY failedAttemptsChanged)
  Q_PROPERTY(int lockoutSeconds READ lockoutSeconds NOTIFY lockoutSecondsChanged)
  Q_PROPERTY(bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
  Q_PROPERTY(bool sensorInitializing READ sensorInitializing NOTIFY sensorInitializingChanged)
  Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
  Q_PROPERTY(int templateCount READ templateCount NOTIFY templateCountChanged)
  Q_PROPERTY(QString operationProgress READ operationProgress NOTIFY operationProgressChanged)
  Q_PROPERTY(bool isAdminMenuAccessible READ isAdminMenuAccessible NOTIFY isAdminMenuAccessibleChanged)
  Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
  Q_PROPERTY(AdminSession* adminSession READ adminSession CONSTANT)
  Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserChanged)
  Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)
  Q_PROPERTY(QString currentUserRoleName READ currentUserRoleName NOTIFY currentUserChanged)
  Q_PROPERTY(QString currentUserRoleColor READ currentUserRoleColor NOTIFY currentUserChanged)
  Q_PROPERTY(UserManager* userManager READ userManager CONSTANT)

public:
  explicit Controller(QObject* parent = nullptr);
  ~Controller() override;

  // Property getters (delegate to managers)
  int authState() const;
  int failedAttempts() const;
  int lockoutSeconds() const;
  bool sensorAvailable() const
  {
    return m_sensorAvailable;
  }
  bool sensorInitializing() const
  {
    return m_sensorInitializing;
  }
  bool isProcessing() const
  {
    return m_isProcessing;
  }
  int templateCount() const
  {
    return m_templateCount;
  }
  QString operationProgress() const
  {
    return m_operationProgress;
  }
  bool isAdminMenuAccessible() const
  {
    return m_isAdminMenuAccessible;
  }
  int scanProgress() const
  {
    return m_scanProgress;
  }
  AdminSession* adminSession() const
  {
    return m_adminSession.get();
  }
  int currentUserId() const;
  QString currentUserName() const;
  QString currentUserRoleName() const;
  QString currentUserRoleColor() const;
  UserManager* userManager() const
  {
    return m_userManager.get();
  }

  // Initialization
  Q_INVOKABLE bool initializeSensor();
  Q_INVOKABLE void cleanupBeforeExit();
  Q_INVOKABLE void synchronizeMetadata();
  Q_INVOKABLE void loadMetadataFromNotepad();
  Q_INVOKABLE void clearNotepad();

  // Authentication operations
  Q_INVOKABLE void startAuthentication();
  Q_INVOKABLE void resetFailedAttempts();

  // Admin access
  Q_INVOKABLE void requestAdminAccess();
  Q_INVOKABLE void revokeAdminAccess();
  Q_INVOKABLE bool isAdminFingerprint(int fingerprintId) const;
  Q_INVOKABLE bool hasPermission(int permission) const;

  // Enrollment operations
  Q_INVOKABLE void enrollFirstBootUser(QString name, int roleType);
  Q_INVOKABLE void enrollFingerprint(int id);
  Q_INVOKABLE void cancelEnrollment();

  // Sensor operations
  Q_INVOKABLE void refreshTemplateCount();

public slots:
  // Sensor operations (unchanged)
  void identifyFingerprint();
  void verifyFingerprint(int id);
  void queryTemplate(int id);
  void deleteFingerprint(int id);
  void clearDatabase();
  void turnLedOn();
  void turnLedOff();
  void setBaudRate(int baudChoice);
  void setSecurityLevel(int level);
  void setPacketSize(int size);
  void softResetSensor();
  void showSystemSettings();

signals:
  // Property change signals
  void authStateChanged();
  void failedAttemptsChanged();
  void lockoutSecondsChanged();
  void sensorAvailableChanged();
  void sensorInitializingChanged();
  void isProcessingChanged();
  void templateCountChanged();
  void operationProgressChanged();
  void scanProgressChanged();
  void currentUserChanged();
  void isAdminMenuAccessibleChanged();

  // Action signals
  void authenticationSuccess();
  void authenticationFailed();
  void lockoutTriggered();
  void lockoutExpired();
  void operationComplete(const QString& message);
  void operationFailed(const QString& error);

  // Admin access signals
  void adminFingerprintRequired();
  void adminAccessGranted();
  void adminAccessDenied(const QString& reason);
  void unauthorizedAccessDetected(const QString& details);
  void adminAccessRevoked();

private slots:
  // Authentication callbacks
  void onAuthenticationSuccess(int fingerId, int confidence);
  void onAuthenticationFailed();
  void onAuthenticationNoFinger();

  // Admin authentication callbacks
  void onAdminFingerprintSuccess(int fingerId, int confidence);
  void onAdminFingerprintFailed(const QString& reason);
  void onAdminFingerprintNoFinger();

  // Enrollment callbacks
  void onEnrollmentComplete(const QString& message);
  void onEnrollmentFailed(const QString& error);

  // Generic operation callbacks
  void onOperationComplete(const QString& message);
  void onOperationFailed(const QString& error);

  // Manager callbacks
  void onAuthManagerLockoutExpired();
  void onEnrollmentSuccess(int userId, const QString& message);
  void onEnrollmentError(const QString& error);

  // Synchronization
  void onEnrolledTemplateIdsReceived(QVector<int> sensorIds);

  // Notepad callbacks
  void onNotepadDataRead(int pageNumber, const QByteArray& data);
  void onNotepadReadFailed(int pageNumber, const QString& error);
  void onNotepadWriteComplete(int pageNumber);
  void onNotepadWriteFailed(int pageNumber, const QString& error);

private:
  // Guard methods (DRY principle)
  bool ensureSensorAvailable();
  bool ensureNotAuthenticating();

  // Property setters
  void setIsProcessing(bool processing);
  void setTemplateCount(int count);
  void setOperationProgress(const QString& progress);
  void setAdminMenuAccessible(bool accessible);
  void setScanProgress(int progress);
  void setSensorInitializing(bool initializing);

  // LED control
  void enableSensorLed();
  void disableSensorLed();

  // Notepad processing
  void processNotepadData();
  void writeMetadataToNotepad(const carbio::TemplateMetadata& meta);
  void clearAllNotepadPages();  // DRY - eliminates 3 duplicate loops
  void readAllNotepadPages();   // DRY - eliminates duplicate code

  // Hardware
  std::unique_ptr<carbio::fingerprint_sensor> m_sensor;
  bool m_sensorAvailable;
  bool m_sensorInitializing;

  // Metadata store (must be first - used by all managers)
  carbio::TemplateMetadataStore m_metadataStore;

  // Managers (Single Responsibility Principle - using std::unique_ptr for RAII)
  // Order matters: UserManager must come before EnrollmentManager
  std::unique_ptr<AuthenticationManager> m_authManager;
  std::unique_ptr<UserManager> m_userManager;
  std::unique_ptr<EnrollmentManager> m_enrollmentManager;
  std::unique_ptr<AdminSession> m_adminSession;

  // Worker thread
  std::unique_ptr<QThread> m_sensorThread;
  std::unique_ptr<SensorWorker> m_sensorWorker;

  // UI state (not managed by managers)
  bool m_isProcessing;
  int m_templateCount;
  QString m_operationProgress;
  bool m_isAdminMenuAccessible;
  int m_scanProgress;

  // Notepad sync state (fixed 16 pages, 32 bytes each - stack allocated, zero heap)
  std::array<std::array<uint8_t, 32>, 16> m_notepadCache;
  int m_notepadPagesRead;
  int m_notepadPagesToRead;

  // Polling interval - 50ms = 20Hz (industry standard for fingerprint sensors)
  // Previous: 5ms = 200Hz was consuming 60% CPU on Raspberry Pi
  static constexpr int POLL_INTERVAL_MS = 50;
};

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
