#ifndef ADMIN_AUTHENTICATOR_H
#define ADMIN_AUTHENTICATOR_H

#include "security_types.h"

#include <QObject>
#include <QString>

#include <array>
#include <chrono>
#include <memory>

namespace carbio::security
{

/**
 * @brief AdminAuthenticator provides cryptographic password verification
 *
 * Features:
 * - PBKDF2-SHA256 password hashing with 100,000 iterations
 * - Cryptographically secure random salt generation
 * - Constant-time comparison to prevent timing attacks
 * - Nonce generation for challenge-response authentication
 * - Rate limiting to prevent brute-force attacks
 */
class AdminAuthenticator : public QObject
{
  Q_OBJECT

public:
  explicit AdminAuthenticator(QObject *parent = nullptr);
  ~AdminAuthenticator() override;

  // Password management
  [[nodiscard]] bool verifyPassword(const QString &password);
  [[nodiscard]] bool setPassword(const QString &newPassword);
  [[nodiscard]] bool hasPasswordSet() const;

  // Challenge-response authentication
  [[nodiscard]] QByteArray generateChallenge();
  [[nodiscard]] bool validateChallenge(const QByteArray &nonce);
  void clearChallenge();

  // Rate limiting
  [[nodiscard]] bool isRateLimited() const;
  [[nodiscard]] int remainingAttempts() const;
  [[nodiscard]] int lockoutSecondsRemaining() const;
  void resetAttempts();

  // Default admin password (MUST be changed on first use)
  static constexpr const char *DEFAULT_PASSWORD = "admin123";

signals:
  void rateLimitTriggered(int lockoutSeconds);
  void passwordVerified();
  void passwordFailed(int remainingAttempts);

private:
  struct Challenge
  {
    std::array<uint8_t, NONCE_SIZE> nonce;
    std::chrono::steady_clock::time_point timestamp;
    bool active;
  };

  // Password hashing
  [[nodiscard]] QByteArray hashPassword(const QString &password, const QByteArray &salt) const;
  [[nodiscard]] QByteArray generateSalt() const;
  [[nodiscard]] bool constantTimeCompare(const QByteArray &a, const QByteArray &b) const;

  // Persistent storage
  void loadPasswordHash();
  void savePasswordHash();
  QString getPasswordFilePath() const;

  // Rate limiting state
  int                                   m_failedAttempts;
  std::chrono::steady_clock::time_point m_lockoutUntil;
  std::chrono::steady_clock::time_point m_lastAttempt;

  // Password storage (hashed)
  QByteArray m_passwordHash;
  QByteArray m_salt;

  // Active challenge
  Challenge m_activeChallenge;

  // Constants
  static constexpr int    PBKDF2_ITERATIONS = 100000;
  static constexpr int    MAX_ATTEMPTS      = 3;
  static constexpr int    RATE_LIMIT_WINDOW = 30; // seconds
  static constexpr size_t PASSWORD_MIN_LENGTH = 8;
};

} // namespace carbio::security

#endif // ADMIN_AUTHENTICATOR_H
