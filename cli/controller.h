#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include "admin_authenticator.h"
#include "audit_logger.h"
#include "auth_types.h"
#include "profile_manager.h"
#include "security_types.h"
#include "session_manager.h"
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \
    "-Wpadded" // Polymorphic types are incompatible with explicit paddings due hidden vtable pointer
#pragma GCC diagnostic ignored "-Wsuggest-final-types"   // QML type requires override and cannot be made final
#pragma GCC diagnostic ignored "-Wsuggest-final-methods" // QML method requires override and cannot be made final
#endif

namespace carbio
{
class fingerprint_sensor;
}

class Controller : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int authState READ authState NOTIFY authStateChanged)
  Q_PROPERTY(int failedAttempts READ failedAttempts NOTIFY failedAttemptsChanged)
  Q_PROPERTY(int lockoutSeconds READ lockoutSeconds NOTIFY lockoutSecondsChanged)
  Q_PROPERTY(int maxLockoutSeconds READ maxLockoutSeconds CONSTANT)
  Q_PROPERTY(QString driverName READ driverName NOTIFY driverNameChanged)
  Q_PROPERTY(bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
  Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
  Q_PROPERTY(int templateCount READ templateCount NOTIFY templateCountChanged)
  Q_PROPERTY(QString operationProgress READ operationProgress NOTIFY operationProgressChanged)
  Q_PROPERTY(bool isAdminMenuAccessible READ isAdminMenuAccessible NOTIFY isAdminMenuAccessibleChanged)
  Q_PROPERTY(QString adminAccessToken READ adminAccessToken NOTIFY adminAccessTokenChanged)

public:
  explicit Controller(QObject *parent = nullptr);
  ~Controller() override;

  // Property getters
  [[nodiscard]] int
  authState() const
  {
    return static_cast<int>(m_authState);
  }
  [[nodiscard]] int
  failedAttempts() const
  {
    return m_failedAttempts;
  }
  [[nodiscard]] int
  lockoutSeconds() const
  {
    return m_lockoutSeconds;
  }
  [[nodiscard]] int
  maxLockoutSeconds() const
  {
    return LOCKOUT_DURATION_SEC;
  }
  [[nodiscard]] QString
  driverName() const
  {
    return m_driverName;
  }
  [[nodiscard]] bool
  sensorAvailable() const
  {
    return m_sensorAvailable;
  }
  [[nodiscard]] bool
  isProcessing() const
  {
    return m_isProcessing;
  }
  [[nodiscard]] int
  templateCount() const
  {
    return m_templateCount;
  }
  [[nodiscard]] QString
  operationProgress() const
  {
    return m_operationProgress;
  }
  [[nodiscard]] bool
  isAdminMenuAccessible() const
  {
    return m_isAdminMenuAccessible;
  }
  [[nodiscard]] QString
  adminAccessToken() const
  {
    return m_adminAccessToken;
  }

  Q_INVOKABLE void startAuthentication();
  Q_INVOKABLE void cancelAuthentication();
  Q_INVOKABLE void resetLockout();
  Q_INVOKABLE bool initializeSensor();
  Q_INVOKABLE void refreshTemplateCount();
  Q_INVOKABLE void requestAdminAccess();
  Q_INVOKABLE void verifyAdminPassword(const QString &password);
  Q_INVOKABLE void startAdminFingerprintScan();
  Q_INVOKABLE void revokeAdminAccess();
  Q_INVOKABLE bool isAdminFingerprint(int fingerprintId) const;

  // Profile management
  Q_INVOKABLE bool addDriver(const QString &name, bool isAdmin);
  Q_INVOKABLE bool deleteDriver(int id);
  Q_INVOKABLE QString getDriverName(int id) const;
  Q_INVOKABLE QVariantList getAllDrivers() const;

public slots:
  void enrollFingerprint(int id);
  void findFingerprint();
  void identifyFingerprint();
  void verifyFingerprint(int id);
  void queryTemplate(int id);
  void deleteFingerprint(int id);
  void clearDatabase();
  void turnLedOn();
  void turnLedOff();
  void toggleLed();
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
  void driverNameChanged();
  void sensorAvailableChanged();

  // Action signals
  void authenticationSuccess(QString driverName);
  void authenticationFailed();
  void lockoutTriggered();
  void isProcessingChanged();
  void templateCountChanged();
  void operationProgressChanged();

  void operationComplete(QString message);
  void operationFailed(QString error);

  // Admin access signals
  void adminPasswordRequired();
  void adminPasswordVerified();
  void adminPasswordFailed(int remainingAttempts);
  void adminFingerprintRequired();
  void adminAccessGranted(QString token);
  void adminAccessDenied(QString reason);
  void unauthorizedAccessDetected(QString details);
  void isAdminMenuAccessibleChanged();
  void adminAccessTokenChanged();
  void adminAccessRevoked();

private slots:
  void onLockoutTick();
  void onAdminFingerprintPoll();

private:
  void    setAuthState(AuthState state);
  void    performAuthentication();
  void    handleAuthenticationFailure();
  void    performAdminFingerprintVerification();
  QString lookupDriverName(uint16_t fingerId);
  void    lockDashboardAfterAdminFailure();

  std::unique_ptr<carbio::fingerprint_sensor> m_sensor;
  AuthState                                   m_authState;
  QString                                     m_driverName;
  int                                         m_failedAttempts;
  int                                         m_lockoutSeconds;
  bool                                        m_sensorAvailable;
  bool                                        m_isProcessing;
  int                                         m_templateCount;
  QString                                     m_operationProgress;
  bool                                        m_manualLedControl;
  bool                                        m_isAdminMenuAccessible;
  QString                                     m_adminAccessToken;

  QTimer *m_scanTimer;
  QTimer *m_lockoutTimer;
  QTimer *m_adminFingerprintTimer;

  // Adaptive polling state
  int m_consecutiveNoFingerAuth;
  int m_consecutiveNoFingerAdmin;

  // Security components
  std::unique_ptr<carbio::security::AdminAuthenticator> m_adminAuth;
  std::unique_ptr<carbio::security::SessionManager>     m_sessionManager;
  std::unique_ptr<carbio::security::AuditLogger>        m_auditLogger;
  std::unique_ptr<carbio::security::ProfileManager>     m_profileManager;

  // Admin authentication state
  enum class AdminAuthPhase : uint8_t
  {
    IDLE,
    PASSWORD_PENDING,
    FINGERPRINT_PENDING,
    COMPLETED
  };
  AdminAuthPhase m_adminAuthPhase;

  void setIsProcessing(bool processing);
  void setTemplateCount(int count);
  void setOperationProgress(const QString &progress);
  void resetSensorState();
  void setAdminMenuAccessible(bool accessible);
  void setAdminAccessToken(const QString &token);

  // Adaptive polling helpers
  void adjustAuthPollingInterval();
  void adjustAdminPollingInterval();
  void resetAuthPollingInterval();
  void resetAdminPollingInterval();

  static constexpr int MAX_ATTEMPTS         = 3;
  static constexpr int LOCKOUT_DURATION_SEC = 20;

  // Adaptive polling configuration
  static constexpr int POLL_INTERVAL_MIN   = 1;  // Minimum interval (ms)
  static constexpr int POLL_INTERVAL_MAX   = 16; // Maximum interval (ms)
  static constexpr int POLL_BACKOFF_FACTOR = 2;  // Exponential backoff multiplier
  static constexpr int POLL_BACKOFF_THRESHOLD = 3; // Start backoff after N misses
};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif // AUTH_CONTROLLER_H
