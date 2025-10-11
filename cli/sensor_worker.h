#ifndef SENSOR_WORKER_H
#define SENSOR_WORKER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <optional>

#include "carbio/device_setting_info.h"

namespace carbio
{
class fingerprint_sensor;
}

class SensorWorker final : public QObject
{
  Q_OBJECT

public:
  explicit SensorWorker(carbio::fingerprint_sensor* sensor, QObject *parent = nullptr);
  ~SensorWorker() override;

public slots:
  // Polling control
  void startAuthenticationPolling(int intervalMs);
  void stopAuthenticationPolling();
  void startAdminPolling(int intervalMs);
  void stopAdminPolling();

  // Performance: Pre-warm cache on startup
  void prewarmCache();

  // Legacy single-shot authentication (for compatibility)
  void performAuthentication();
  void performAdminAuthentication();

  // Enrollment and identification
  void enrollFingerprint(int id);
  void findFingerprint();
  void identifyFingerprint();
  void verifyFingerprint(int id);
  void queryTemplate(int id);

  // Database operations
  void deleteFingerprint(int id);
  void clearDatabase();
  void refreshTemplateCount();

  // LED operations
  void turnLedOn();
  void turnLedOff();

  // Configuration operations
  void setBaudRate(int baudChoice);
  void setSecurityLevel(int level);
  void setPacketSize(int size);
  void softResetSensor();
  void showSystemSettings();

  // Utility operations
  void resetSensorState();

private slots:
  // Polling tick handlers (run on worker thread - no context switching!)
  void onAuthPollTick();
  void onAdminPollTick();
  void onOperationPollTick();

  // Helper methods for async operations
  void completeFindOperation();
  void completeIdentifyOperation();
  void completeVerifyOperation(int id);
  void continueEnrollmentOperation(int id);

signals:
  // Authentication results
  void authenticationSuccess(int fingerId, int confidence, QString driverName);
  void authenticationFailed();
  void authenticationNoFinger();

  // Admin authentication results
  void adminFingerprintSuccess(int fingerId, int confidence);
  void adminFingerprintFailed(QString reason);
  void adminFingerprintNoFinger();

  // Enrollment signals
  void enrollmentProgress(QString message);
  void enrollmentComplete(QString message);
  void enrollmentFailed(QString error);

  // Operation result signals
  void operationComplete(QString message);
  void operationFailed(QString error);

  // Progress signals
  void progressUpdate(QString message);
  void templateCountUpdated(int count);
  void scanProgressUpdate(int percentage);

private:
  carbio::fingerprint_sensor* m_sensor;  // Non-owning pointer (Controller owns it)

  // Polling timers
  QTimer* m_authPollTimer;          // Authentication polling timer
  QTimer* m_adminPollTimer;         // Admin authentication polling timer
  QTimer* m_operationPollTimer;     // Operation polling timer (for async dialog ops)

  // Operation state tracking
  enum class OperationType {
    NONE,
    FIND,
    IDENTIFY,
    VERIFY,
    ENROLL
  };
  OperationType m_currentOperation;
  int m_operationParameter;  // Used for verify (id) and enroll (id)
  int m_enrollmentStage;     // Track multi-stage enrollment progress
  std::optional<carbio::device_setting_info> m_cachedSettings;

  // Inline for zero-cost abstraction in hot path
  inline carbio::device_setting_info getDeviceSettings()
  {
    if (!m_cachedSettings.has_value()) [[unlikely]]
    {
      prewarmCache();
    }
    return m_cachedSettings.value();
  }
};

#endif // SENSOR_WORKER_H
