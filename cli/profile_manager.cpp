#include "profile_manager.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QStandardPaths>
#include <QDebug>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>

namespace carbio::security
{

ProfileManager::ProfileManager(QObject *parent)
  : QObject(parent)
{
  // Use XDG-compliant config directory
  QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (configDir.isEmpty())
  {
    configDir = QDir::homePath() + "/.carbio";
  }

  QDir dir(configDir);
  if (!dir.exists())
  {
    dir.mkpath(".");
  }

  m_profilesPath = dir.filePath("profiles.dat");
  qDebug() << "Profile storage path:" << m_profilesPath;
}

ProfileManager::~ProfileManager() = default;

QByteArray ProfileManager::readCpuSerial()
{
  QFile cpuInfo("/proc/cpuinfo");
  if (!cpuInfo.open(QIODevice::ReadOnly))
  {
    qWarning() << "Failed to read CPU info, using fallback";
    return QByteArray("FALLBACK_SERIAL");
  }

  while (!cpuInfo.atEnd())
  {
    QString line = QString::fromLatin1(cpuInfo.readLine());
    if (line.startsWith("Serial"))
    {
      // Format: "Serial : 00000000abcd1234"
      QStringList parts = line.split(":");
      if (parts.size() >= 2)
      {
        QString serial = parts[1].trimmed();
        qDebug() << "CPU Serial:" << serial;
        return serial.toLatin1();
      }
    }
  }

  qWarning() << "CPU serial not found, using fallback";
  return QByteArray("FALLBACK_SERIAL");
}

QByteArray ProfileManager::readEncryptionKeyFile()
{
  QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (configDir.isEmpty())
  {
    configDir = QDir::homePath() + "/.carbio";
  }

  QString keyPath = configDir + "/encryption_key.dat";
  QFile keyFile(keyPath);

  if (!keyFile.open(QIODevice::ReadOnly))
  {
    qWarning() << "Encryption key file not found, generating new one";

    // Generate new random key
    QByteArray newKey(AES_KEY_SIZE, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(newKey.data()), AES_KEY_SIZE) != 1)
    {
      qCritical() << "Failed to generate random key";
      return QByteArray();
    }

    // Save it
    if (keyFile.open(QIODevice::WriteOnly))
    {
      keyFile.write(newKey);
      keyFile.close();
      qInfo() << "Generated new encryption key";
      return newKey;
    }
    else
    {
      qCritical() << "Failed to save encryption key";
      return QByteArray();
    }
  }

  QByteArray key = keyFile.readAll();
  qDebug() << "Loaded encryption key (" << key.size() << "bytes)";
  return key;
}

QByteArray ProfileManager::deriveEncryptionKey()
{
  // Hardware-bound key derivation
  QByteArray cpuSerial = readCpuSerial();
  QByteArray diskKey = readEncryptionKeyFile();

  if (diskKey.isEmpty())
  {
    qCritical() << "Failed to derive encryption key";
    return QByteArray();
  }

  // Combine hardware ID + disk key using PBKDF2
  QByteArray salt = cpuSerial + QByteArrayLiteral("carbio_profiles_v1");

  // Use OpenSSL's PKCS5_PBKDF2_HMAC for PBKDF2
  QByteArray derivedKey(AES_KEY_SIZE, 0);
  if (PKCS5_PBKDF2_HMAC(
        diskKey.constData(), diskKey.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        PBKDF2_ITERATIONS,
        EVP_sha256(),
        AES_KEY_SIZE,
        reinterpret_cast<unsigned char*>(derivedKey.data())) != 1)
  {
    qCritical() << "PBKDF2 key derivation failed";
    return QByteArray();
  }

  return derivedKey;
}

QByteArray ProfileManager::encryptData(const QByteArray &plaintext, const QByteArray &key)
{
  if (key.size() != AES_KEY_SIZE)
  {
    qCritical() << "Invalid key size for encryption";
    return QByteArray();
  }

  // Generate random IV (12 bytes for GCM)
  unsigned char iv[12];
  if (RAND_bytes(iv, sizeof(iv)) != 1)
  {
    qCritical() << "Failed to generate IV";
    return QByteArray();
  }

  // Prepare output buffer
  QByteArray ciphertext;
  ciphertext.resize(plaintext.size() + 16); // +16 for GCM tag

  // AES-256-GCM encryption
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    qCritical() << "Failed to create cipher context";
    return QByteArray();
  }

  int len = 0;
  int ciphertext_len = 0;

  // Initialize encryption
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                         reinterpret_cast<const unsigned char*>(key.constData()), iv) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Encryption init failed";
    return QByteArray();
  }

  // Encrypt data
  if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                        reinterpret_cast<const unsigned char*>(plaintext.constData()),
                        plaintext.size()) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Encryption update failed";
    return QByteArray();
  }
  ciphertext_len = len;

  // Finalize encryption
  if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Encryption finalization failed";
    return QByteArray();
  }
  ciphertext_len += len;

  // Get GCM tag
  unsigned char tag[16];
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Failed to get GCM tag";
    return QByteArray();
  }

  EVP_CIPHER_CTX_free(ctx);

  // Format: IV (12 bytes) + Ciphertext + Tag (16 bytes)
  QByteArray result;
  result.append(reinterpret_cast<const char*>(iv), sizeof(iv));
  result.append(ciphertext.constData(), ciphertext_len);
  result.append(reinterpret_cast<const char*>(tag), sizeof(tag));

  return result;
}

QByteArray ProfileManager::decryptData(const QByteArray &ciphertext, const QByteArray &key)
{
  if (key.size() != AES_KEY_SIZE)
  {
    qCritical() << "Invalid key size for decryption";
    return QByteArray();
  }

  if (ciphertext.size() < 12 + 16) // IV + Tag minimum
  {
    qCritical() << "Ciphertext too short";
    return QByteArray();
  }

  // Extract IV, encrypted data, and tag
  const unsigned char *iv = reinterpret_cast<const unsigned char*>(ciphertext.constData());
  const unsigned char *encrypted = iv + 12;
  int encrypted_len = ciphertext.size() - 12 - 16;
  const unsigned char *tag = encrypted + encrypted_len;

  // Prepare output buffer
  QByteArray plaintext;
  plaintext.resize(encrypted_len);

  // AES-256-GCM decryption
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    qCritical() << "Failed to create cipher context";
    return QByteArray();
  }

  int len = 0;
  int plaintext_len = 0;

  // Initialize decryption
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                         reinterpret_cast<const unsigned char*>(key.constData()), iv) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Decryption init failed";
    return QByteArray();
  }

  // Decrypt data
  if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                        encrypted, encrypted_len) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Decryption update failed";
    return QByteArray();
  }
  plaintext_len = len;

  // Set expected tag
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<unsigned char*>(tag)) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Failed to set GCM tag";
    return QByteArray();
  }

  // Finalize and verify tag
  if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) <= 0)
  {
    EVP_CIPHER_CTX_free(ctx);
    qCritical() << "Decryption failed or tag verification failed - data may be tampered!";
    return QByteArray();
  }
  plaintext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  plaintext.resize(plaintext_len);
  return plaintext;
}

void ProfileManager::secureClearMemory(QByteArray &data)
{
  // Overwrite with zeros before clearing
  OPENSSL_cleanse(data.data(), data.size());
  data.clear();
}

QByteArray ProfileManager::serializeProfiles() const
{
  QJsonArray profilesArray;

  for (const Profile &profile : m_profiles)
  {
    QJsonObject profileObj;
    profileObj["id"] = static_cast<int>(profile.id);
    profileObj["name"] = profile.name;
    profileObj["isAdmin"] = profile.isAdmin;
    profileObj["createdAt"] = profile.createdAt.toString(Qt::ISODate);
    profileObj["modifiedAt"] = profile.modifiedAt.toString(Qt::ISODate);

    profilesArray.append(profileObj);
  }

  QJsonObject root;
  root["version"] = 1;
  root["profiles"] = profilesArray;

  QJsonDocument doc(root);
  return doc.toJson(QJsonDocument::Compact);
}

bool ProfileManager::deserializeProfiles(const QByteArray &data)
{
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject())
  {
    qCritical() << "Invalid JSON format";
    return false;
  }

  QJsonObject root = doc.object();
  int version = root["version"].toInt();
  if (version != 1)
  {
    qCritical() << "Unsupported profile version:" << version;
    return false;
  }

  QJsonArray profilesArray = root["profiles"].toArray();
  m_profiles.clear();

  for (const QJsonValue &value : profilesArray)
  {
    QJsonObject obj = value.toObject();
    Profile profile;
    profile.id = static_cast<uint16_t>(obj["id"].toInt());
    profile.name = obj["name"].toString();
    profile.isAdmin = obj["isAdmin"].toBool();
    profile.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    profile.modifiedAt = QDateTime::fromString(obj["modifiedAt"].toString(), Qt::ISODate);

    m_profiles.append(profile);
  }

  qInfo() << "Loaded" << m_profiles.size() << "profiles";
  return true;
}

bool ProfileManager::loadProfiles()
{
  QFile file(m_profilesPath);

  if (!file.exists())
  {
    qInfo() << "No existing profiles file, starting fresh";
    m_profiles.clear();
    return true;
  }

  if (!file.open(QIODevice::ReadOnly))
  {
    QString error = "Failed to open profiles file: " + file.errorString();
    qCritical() << error;
    emit loadError(error);
    return false;
  }

  QByteArray encryptedData = file.readAll();
  file.close();

  // Derive key and decrypt
  QByteArray key = deriveEncryptionKey();
  if (key.isEmpty())
  {
    emit loadError("Failed to derive encryption key");
    return false;
  }

  QByteArray decrypted = decryptData(encryptedData, key);
  secureClearMemory(key);

  if (decrypted.isEmpty())
  {
    emit loadError("Failed to decrypt profiles - data may be corrupted or tampered");
    return false;
  }

  bool success = deserializeProfiles(decrypted);
  secureClearMemory(decrypted);

  return success;
}

bool ProfileManager::saveProfiles()
{
  // Serialize profiles to JSON
  QByteArray plaintext = serializeProfiles();

  // Derive key and encrypt
  QByteArray key = deriveEncryptionKey();
  if (key.isEmpty())
  {
    emit saveError("Failed to derive encryption key");
    return false;
  }

  QByteArray encrypted = encryptData(plaintext, key);
  secureClearMemory(key);
  secureClearMemory(plaintext);

  if (encrypted.isEmpty())
  {
    emit saveError("Failed to encrypt profiles");
    return false;
  }

  // Atomic write: write to temp file, then rename
  QString tempPath = m_profilesPath + ".tmp";
  QFile tempFile(tempPath);

  if (!tempFile.open(QIODevice::WriteOnly))
  {
    QString error = "Failed to open temp file: " + tempFile.errorString();
    qCritical() << error;
    emit saveError(error);
    return false;
  }

  if (tempFile.write(encrypted) != encrypted.size())
  {
    tempFile.remove();
    emit saveError("Failed to write encrypted data");
    return false;
  }

  tempFile.close();

  // Atomic rename
  QFile::remove(m_profilesPath);
  if (!QFile::rename(tempPath, m_profilesPath))
  {
    emit saveError("Failed to rename temp file");
    return false;
  }

  qInfo() << "Saved" << m_profiles.size() << "profiles to encrypted storage";
  return true;
}

bool ProfileManager::addProfile(const QString &name, bool isAdmin, uint16_t *assignedId)
{
  if (name.trimmed().isEmpty())
  {
    qWarning() << "Cannot add profile with empty name";
    return false;
  }

  uint16_t id = getNextAvailableId();
  if (id > MAX_ID)
  {
    qWarning() << "No available profile slots (max 128)";
    return false;
  }

  Profile profile;
  profile.id = id;
  profile.name = name.trimmed();
  profile.isAdmin = isAdmin;
  profile.createdAt = QDateTime::currentDateTime();
  profile.modifiedAt = profile.createdAt;

  m_profiles.append(profile);

  if (assignedId)
  {
    *assignedId = id;
  }

  emit profileAdded(id, profile.name);
  qInfo() << "Added profile:" << profile.name << "(ID:" << id << ", Admin:" << isAdmin << ")";

  return saveProfiles();
}

bool ProfileManager::deleteProfile(uint16_t id)
{
  for (int i = 0; i < m_profiles.size(); ++i)
  {
    if (m_profiles[i].id == id)
    {
      QString name = m_profiles[i].name;
      m_profiles.removeAt(i);
      emit profileDeleted(id);
      qInfo() << "Deleted profile:" << name << "(ID:" << id << ")";
      return saveProfiles();
    }
  }

  qWarning() << "Profile ID" << id << "not found";
  return false;
}

bool ProfileManager::updateProfile(uint16_t id, const QString &newName, bool isAdmin)
{
  for (Profile &profile : m_profiles)
  {
    if (profile.id == id)
    {
      profile.name = newName.trimmed();
      profile.isAdmin = isAdmin;
      profile.modifiedAt = QDateTime::currentDateTime();
      emit profileUpdated(id);
      qInfo() << "Updated profile ID" << id << "to:" << newName;
      return saveProfiles();
    }
  }

  qWarning() << "Profile ID" << id << "not found";
  return false;
}

ProfileManager::Profile *ProfileManager::getProfile(uint16_t id)
{
  for (Profile &profile : m_profiles)
  {
    if (profile.id == id)
    {
      return &profile;
    }
  }
  return nullptr;
}

QString ProfileManager::getDriverName(uint16_t id) const
{
  for (const Profile &profile : m_profiles)
  {
    if (profile.id == id)
    {
      return profile.name;
    }
  }
  return QString("Unknown (ID %1)").arg(id);
}

bool ProfileManager::isAdminId(uint16_t id) const
{
  for (const Profile &profile : m_profiles)
  {
    if (profile.id == id)
    {
      return profile.isAdmin;
    }
  }
  return false;
}

uint16_t ProfileManager::getNextAvailableId() const
{
  // Build set of used IDs
  QSet<uint16_t> usedIds;
  for (const Profile &profile : m_profiles)
  {
    usedIds.insert(profile.id);
  }

  // Find first available ID
  for (uint16_t id = MIN_ID; id <= MAX_ID; ++id)
  {
    if (!usedIds.contains(id))
    {
      return id;
    }
  }

  return MAX_ID + 1; // Invalid - all slots used
}

QVector<ProfileManager::Profile> ProfileManager::getAllProfiles() const
{
  return m_profiles;
}

int ProfileManager::getProfileCount() const
{
  return m_profiles.size();
}

bool ProfileManager::isValidId(uint16_t id) const
{
  return id >= MIN_ID && id <= MAX_ID;
}

bool ProfileManager::profileExists(uint16_t id) const
{
  for (const Profile &profile : m_profiles)
  {
    if (profile.id == id)
    {
      return true;
    }
  }
  return false;
}

} // namespace carbio::security
