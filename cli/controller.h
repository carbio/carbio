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

#include "auth_types.h"
#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <memory>

// Admin authentication constants
namespace carbio::security {
constexpr uint16_t ADMIN_ID_MIN = 0;
constexpr uint16_t ADMIN_ID_MAX = 2;
constexpr uint16_t MIN_ADMIN_CONFIDENCE = 150;
} // namespace carbio::security

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored                                                 \
    "-Wpadded" // Polymorphic types are incompatible with explicit paddings due
               // hidden vtable pointer
#pragma GCC diagnostic ignored                                                 \
    "-Wsuggest-final-types" // QML type requires override and cannot be made
                            // final
#pragma GCC diagnostic ignored                                                 \
    "-Wsuggest-final-methods" // QML method requires override and cannot be made
                              // final
#endif

namespace carbio::fingerprint {
class fingerprint_sensor;
}

class SensorWorker;
class QThread;

class Controller : public QObject {
  Q_OBJECT

  Q_PROPERTY(int authState READ authState NOTIFY authStateChanged)
  Q_PROPERTY(
      int failedAttempts READ failedAttempts NOTIFY failedAttemptsChanged)
  Q_PROPERTY(
      int lockoutSeconds READ lockoutSeconds NOTIFY lockoutSecondsChanged)
  Q_PROPERTY(int maxLockoutSeconds READ maxLockoutSeconds CONSTANT)
  Q_PROPERTY(
      bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
  Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
  Q_PROPERTY(int templateCount READ templateCount NOTIFY templateCountChanged)
  Q_PROPERTY(QString operationProgress READ operationProgress NOTIFY
                 operationProgressChanged)
  Q_PROPERTY(bool isAdminMenuAccessible READ isAdminMenuAccessible NOTIFY
                 isAdminMenuAccessibleChanged)
  Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)

public:
  explicit Controller(QObject *parent = nullptr);
  ~Controller() override;

  // Property getters
  [[nodiscard]] int authState() const { return static_cast<int>(m_authState); }
  [[nodiscard]] int failedAttempts() const { return m_failedAttempts; }
  [[nodiscard]] int lockoutSeconds() const { return m_lockoutSeconds; }
  [[nodiscard]] int maxLockoutSeconds() const { return LOCKOUT_DURATION_SEC; }
  [[nodiscard]] bool sensorAvailable() const { return m_sensorAvailable; }
  [[nodiscard]] bool isProcessing() const { return m_isProcessing; }
  [[nodiscard]] int templateCount() const { return m_templateCount; }
  [[nodiscard]] QString operationProgress() const {
    return m_operationProgress;
  }
  [[nodiscard]] bool isAdminMenuAccessible() const {
    return m_isAdminMenuAccessible;
  }
  [[nodiscard]] int scanProgress() const { return m_scanProgress; }

  Q_INVOKABLE void startAuthentication();
  Q_INVOKABLE bool initializeSensor();
  Q_INVOKABLE void refreshTemplateCount();
  Q_INVOKABLE void requestAdminAccess();
  Q_INVOKABLE void revokeAdminAccess();
  Q_INVOKABLE bool isAdminFingerprint(int fingerprintId) const;

  // Profile management
  Q_INVOKABLE void enrollFingerprint(int id);
  Q_INVOKABLE void cleanupBeforeExit();

public slots:
  void findFingerprint();
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

  // Action signals
  void authenticationSuccess();
  void authenticationFailed();
  void lockoutTriggered();
  void isProcessingChanged();
  void templateCountChanged();
  void operationProgressChanged();

  void operationComplete(QString message);
  void operationFailed(QString error);

  // Admin access signals
  void adminFingerprintRequired();
  void adminAccessGranted();
  void adminAccessDenied(QString reason);
  void unauthorizedAccessDetected(QString details);
  void isAdminMenuAccessibleChanged();
  void adminAccessRevoked();
  void scanProgressChanged();

private slots:
  void onLockoutTick();

  // Worker response handlers
  void onAuthenticationSuccess(int fingerId, int confidence);
  void onAuthenticationFailed();
  void onAuthenticationNoFinger();
  void onAdminFingerprintSuccess(int fingerId, int confidence);
  void onAdminFingerprintFailed(QString reason);
  void onAdminFingerprintNoFinger();
  void onEnrollmentComplete(QString message);
  void onEnrollmentFailed(QString error);
  void onOperationComplete(QString message);
  void onOperationFailed(QString error);

private:
  void setAuthState(AuthState state);
  void performAuthentication();
  void handleAuthenticationFailure();
  void lockDashboardAfterAdminFailure();

  std::unique_ptr<carbio::fingerprint::fingerprint_sensor> m_sensor;
  AuthState m_authState;
  int m_failedAttempts;
  int m_lockoutSeconds;
  bool m_sensorAvailable;
  bool m_isProcessing;
  int m_templateCount;
  QString m_operationProgress;
  bool m_isAdminMenuAccessible;
  int m_scanProgress;

  QTimer *m_lockoutTimer; // Still on main thread (UI countdown timer)

  // Worker thread for sensor operations
  QThread *m_sensorThread;
  SensorWorker *m_sensorWorker;

  void setIsProcessing(bool processing);
  void setTemplateCount(int count);
  void setOperationProgress(const QString &progress);
  void setAdminMenuAccessible(bool accessible);
  void setScanProgress(int progress);

  void enableSensorAutoFingerDetection();
  void disableSensorAutoFingerDetection();

  // Security lockout configuration
  static constexpr int MAX_ATTEMPTS = 3;
  static constexpr int LOCKOUT_DURATION_SEC = 20;

  // Adaptive polling intervals for worker thread
  // CAVEAT: These control authentication responsiveness vs CPU usage tradeoff
  // Lower intervals = faster response but higher CPU load on worker thread
  static constexpr int POLL_INTERVAL_ULTRA =
      1; // 1ms = 1000 Hz - burst mode after failure (instant retry)
  static constexpr int POLL_INTERVAL_FAST =
      3; // 3ms = 333 Hz - active authentication (user engaged)
  static constexpr int POLL_INTERVAL_NORMAL =
      5; // 5ms = 200 Hz - background monitoring (standby)
};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
