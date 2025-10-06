#ifndef AUDIT_LOGGER_H
#define AUDIT_LOGGER_H

#include "security_types.h"

#include <QObject>

#include <chrono>
#include <memory>

namespace carbio::security
{

/**
 * @brief AuditLogger provides tamper-proof security event logging
 *
 * Features:
 * - Hash-chained log entries (blockchain-inspired)
 * - AES-256-CBC encryption for log entries
 * - Append-only file format
 * - Tamper detection via hash chain verification
 * - Timestamped with monotonic clock
 */
class AuditLogger : public QObject
{
  Q_OBJECT

public:
  explicit AuditLogger(QObject *parent = nullptr);
  ~AuditLogger() override;

  /**
   * @brief Log a security event
   * @param event Type of security event
   * @param userId User/fingerprint ID involved
   * @param result Result of the operation
   * @param details Additional context (optional)
   */
  void logEvent(SecurityEvent event, uint16_t userId, AuthResult result, const QString &details = QString());

  /**
   * @brief Log admin access attempt
   * @param userId Fingerprint ID
   * @param passwordValid Was password correct?
   * @param fingerprintValid Was fingerprint valid admin?
   * @param granted Was access granted?
   */
  void logAdminAccess(uint16_t userId, bool passwordValid, bool fingerprintValid, bool granted);

  /**
   * @brief Log unauthorized access
   * @param userId Fingerprint ID (if known)
   * @param details Description
   */
  void logUnauthorizedAccess(uint16_t userId, const QString &details);

  /**
   * @brief Verify integrity of audit log
   * @return true if hash chain is intact, false if tampered
   */
  [[nodiscard]] bool verifyIntegrity() const;

  /**
   * @brief Get recent log entries (for admin review)
   * @param count Number of entries to retrieve
   * @return List of recent audit entries
   */
  [[nodiscard]] QStringList getRecentEntries(int count = 50) const;

  /**
   * @brief Export audit log for forensic analysis
   * @param outputPath Path to export file
   * @return true on success
   */
  [[nodiscard]] bool exportLog(const QString &outputPath) const;

signals:
  void eventLogged(SecurityEvent event, uint16_t userId);
  void unauthorizedAccessDetected(QString details);
  void integrityCheckFailed();

private:
  /**
   * @brief Initialize encryption key and hash chain
   */
  void initialize();

  /**
   * @brief Load encryption key from secure storage
   */
  void loadEncryptionKey();

  /**
   * @brief Save encryption key to secure storage
   */
  void saveEncryptionKey();

  /**
   * @brief Compute hash of log entry
   * @param entry The audit entry
   * @return SHA-256 hash
   */
  [[nodiscard]] QByteArray computeEntryHash(const AuditEntry &entry) const;

  /**
   * @brief Encrypt log entry for storage
   * @param plaintext Raw log data
   * @return Encrypted data with IV prepended
   */
  [[nodiscard]] QByteArray encryptEntry(const QByteArray &plaintext) const;

  /**
   * @brief Decrypt log entry from storage
   * @param ciphertext Encrypted data with IV
   * @return Decrypted log data
   */
  [[nodiscard]] QByteArray decryptEntry(const QByteArray &ciphertext) const;

  /**
   * @brief Append entry to log file
   * @param entry The audit entry to write
   */
  void appendToLog(const AuditEntry &entry);

  /**
   * @brief Read all log entries
   * @return List of all audit entries
   */
  [[nodiscard]] QVector<AuditEntry> readAllEntries() const;

  /**
   * @brief Get path to audit log file
   */
  [[nodiscard]] QString getLogFilePath() const;

  /**
   * @brief Get path to encryption key file
   */
  [[nodiscard]] QString getKeyFilePath() const;

  /**
   * @brief Format entry for human-readable display
   */
  [[nodiscard]] QString formatEntry(const AuditEntry &entry) const;

  // Encryption key for AES-256
  QByteArray m_encryptionKey;

  // Last hash in the chain (for integrity)
  std::array<uint8_t, HASH_SIZE> m_lastHash;

  // Entry counter
  uint64_t m_entryCount;

  static constexpr size_t ENCRYPTION_KEY_SIZE = 32; // AES-256
  static constexpr size_t IV_SIZE = 16;             // AES block size
};

} // namespace carbio::security

#endif // AUDIT_LOGGER_H
