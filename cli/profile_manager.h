#ifndef PROFILE_MANAGER_H
#define PROFILE_MANAGER_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <cstdint>

namespace carbio::security
{

/**
 * @brief Secure profile storage with hardware-bound encryption
 *
 * Stores user profiles (name, fingerprint ID, admin status) in encrypted format.
 * Encryption key is derived from:
 * - Raspberry Pi CPU serial number (hardware binding)
 * - Existing encryption_key.dat file
 * - PBKDF2 key derivation for additional security
 *
 * This ensures encrypted data is only decryptable on the specific hardware.
 */
class ProfileManager : public QObject
{
  Q_OBJECT

public:
  struct Profile
  {
    uint16_t id;          ///< Fingerprint sensor ID (0-127)
    QString name;         ///< Driver name (e.g., "Sarah", "Peter")
    bool isAdmin;         ///< Admin privileges (IDs 0-2 should be admin)
    QDateTime createdAt;  ///< Profile creation timestamp
    QDateTime modifiedAt; ///< Last modification timestamp
  };

  explicit ProfileManager(QObject *parent = nullptr);
  ~ProfileManager() override;

  // Profile management
  bool loadProfiles();
  bool saveProfiles();
  bool addProfile(const QString &name, bool isAdmin, uint16_t *assignedId = nullptr);
  bool deleteProfile(uint16_t id);
  bool updateProfile(uint16_t id, const QString &newName, bool isAdmin);

  // Profile queries
  Profile *getProfile(uint16_t id);
  QString getDriverName(uint16_t id) const;
  bool isAdminId(uint16_t id) const;
  uint16_t getNextAvailableId() const;
  QVector<Profile> getAllProfiles() const;
  int getProfileCount() const;

  // Validation
  bool isValidId(uint16_t id) const;
  bool profileExists(uint16_t id) const;

signals:
  void profileAdded(uint16_t id, QString name);
  void profileDeleted(uint16_t id);
  void profileUpdated(uint16_t id);
  void loadError(QString error);
  void saveError(QString error);

private:
  // Encryption
  QByteArray deriveEncryptionKey();
  QByteArray encryptData(const QByteArray &plaintext, const QByteArray &key);
  QByteArray decryptData(const QByteArray &ciphertext, const QByteArray &key);

  // Hardware binding
  QByteArray readCpuSerial();
  QByteArray readEncryptionKeyFile();

  // Serialization
  QByteArray serializeProfiles() const;
  bool deserializeProfiles(const QByteArray &data);

  // Security
  void secureClearMemory(QByteArray &data);

  QVector<Profile> m_profiles;
  QString m_profilesPath;

  static constexpr uint16_t MIN_ID = 0;
  static constexpr uint16_t MAX_ID = 127;
  static constexpr int PBKDF2_ITERATIONS = 100000;
  static constexpr int AES_KEY_SIZE = 32; // AES-256
};

} // namespace carbio::security

#endif // PROFILE_MANAGER_H
