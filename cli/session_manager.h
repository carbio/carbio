#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "security_types.h"

#include <QObject>
#include <QTimer>

#include <chrono>
#include <memory>
#include <optional>

namespace carbio::security
{

/**
 * @brief SessionManager handles cryptographic session token lifecycle
 *
 * Features:
 * - HMAC-SHA256 signed tokens to prevent forgery
 * - Time-based expiry (5 minutes default)
 * - Single-use tokens (burned after validation)
 * - Automatic cleanup of expired tokens
 */
class SessionManager : public QObject
{
  Q_OBJECT

public:
  explicit SessionManager(QObject *parent = nullptr);
  ~SessionManager() override;

  /**
   * @brief Generate a new session token for an admin user
   * @param adminId The admin's fingerprint ID (must be 0-2)
   * @return Base64-encoded token string for transmission to QML
   */
  [[nodiscard]] QString generateToken(uint16_t adminId);

  /**
   * @brief Validate a session token
   * @param tokenString Base64-encoded token from QML
   * @param adminId Expected admin ID (for additional validation)
   * @return AuthResult indicating success or failure reason
   */
  [[nodiscard]] AuthResult validateToken(const QString &tokenString, uint16_t adminId);

  /**
   * @brief Check if a valid session exists for an admin
   * @param adminId Admin fingerprint ID
   * @return true if valid unexpired token exists
   */
  [[nodiscard]] bool hasValidSession(uint16_t adminId) const;

  /**
   * @brief Revoke all active sessions (emergency logout)
   */
  void revokeAllSessions();

  /**
   * @brief Revoke a specific admin's session
   * @param adminId Admin fingerprint ID
   */
  void revokeSession(uint16_t adminId);

  /**
   * @brief Get remaining session time in seconds
   * @param adminId Admin fingerprint ID
   * @return Seconds remaining, or 0 if no valid session
   */
  [[nodiscard]] int sessionTimeRemaining(uint16_t adminId) const;

signals:
  void tokenGenerated(uint16_t adminId);
  void tokenValidated(uint16_t adminId);
  void tokenExpired(uint16_t adminId);
  void tokenRevoked(uint16_t adminId);
  void sessionExpiring(uint16_t adminId, int secondsRemaining);

private:
  /**
   * @brief Generate HMAC-SHA256 signature for token
   * @param token Raw token bytes
   * @param timestamp Unix timestamp
   * @param adminId Admin fingerprint ID
   * @return HMAC signature
   */
  [[nodiscard]] QByteArray generateHMAC(const QByteArray &token, uint64_t timestamp, uint16_t adminId) const;

  /**
   * @brief Verify HMAC signature
   * @param token Raw token bytes
   * @param timestamp Unix timestamp
   * @param adminId Admin fingerprint ID
   * @param signature HMAC to verify
   * @return true if signature is valid
   */
  [[nodiscard]] bool verifyHMAC(const QByteArray &token, uint64_t timestamp, uint16_t adminId,
                                 const QByteArray &signature) const;

  /**
   * @brief Clean up expired tokens
   */
  void cleanupExpiredTokens();

  /**
   * @brief Load HMAC secret key from persistent storage
   */
  void loadSecretKey();

  /**
   * @brief Save HMAC secret key to persistent storage
   */
  void saveSecretKey();

  /**
   * @brief Get path to secret key file
   */
  [[nodiscard]] QString getSecretKeyPath() const;

  /**
   * @brief Constant-time comparison for cryptographic operations
   */
  [[nodiscard]] bool constantTimeCompare(const QByteArray &a, const QByteArray &b) const;

  // Active sessions (indexed by admin ID)
  std::optional<SessionToken> m_activeSessions[ADMIN_ID_MAX + 1];

  // HMAC secret key for signing tokens
  QByteArray m_secretKey;

  // Cleanup timer
  QTimer *m_cleanupTimer;

  static constexpr size_t SECRET_KEY_SIZE = 32;
  static constexpr int    CLEANUP_INTERVAL_MS = 60000; // 1 minute
  static constexpr int    EXPIRY_WARNING_SECONDS = 60; // Warn 1 minute before expiry
};

} // namespace carbio::security

#endif // SESSION_MANAGER_H
