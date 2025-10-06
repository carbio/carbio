#include "audit_logger.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QStandardPaths>

#include <algorithm>

using namespace carbio::security;
using namespace std::chrono;

AuditLogger::AuditLogger(QObject *parent)
  : QObject(parent)
  , m_lastHash{}
  , m_entryCount(0)
{
  initialize();
}

AuditLogger::~AuditLogger()
{
  // Clear sensitive data
  m_encryptionKey.fill(0);
  m_lastHash.fill(0);
}

void AuditLogger::logEvent(SecurityEvent event, uint16_t userId, AuthResult result, const QString &details)
{
  AuditEntry entry;
  entry.timestamp = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  entry.event = event;
  entry.userId = userId;
  entry.result = result;
  entry.ipAddress = "local"; // Could be extended for network access
  entry.details = details.toStdString();
  entry.prevHash = m_lastHash;

  // Compute hash of this entry
  QByteArray hash = computeEntryHash(entry);
  std::copy_n(hash.constData(), HASH_SIZE, entry.entryHash.begin());

  // Update chain
  m_lastHash = entry.entryHash;
  m_entryCount++;

  // Write to log
  appendToLog(entry);

  qDebug() << "Audit log entry" << m_entryCount << ":" << securityEventToString(event).data() << "for user" << userId
           << "-" << authResultToString(result).data();

  emit eventLogged(event, userId);

  if (event == SecurityEvent::UNAUTHORIZED_ACCESS)
  {
    emit unauthorizedAccessDetected(details);
  }
}

void AuditLogger::logAdminAccess(uint16_t userId, bool passwordValid, bool fingerprintValid, bool granted)
{
  QString details = QString("Password: %1, Fingerprint: %2")
                        .arg(passwordValid ? "VALID" : "INVALID")
                        .arg(fingerprintValid ? "VALID ADMIN" : "NOT ADMIN");

  SecurityEvent event = granted ? SecurityEvent::ADMIN_ACCESS_GRANTED : SecurityEvent::ADMIN_ACCESS_DENIED;

  AuthResult result = granted ? AuthResult::SUCCESS : (fingerprintValid ? AuthResult::INVALID_PASSWORD : AuthResult::NOT_ADMIN);

  logEvent(event, userId, result, details);
}

void AuditLogger::logUnauthorizedAccess(uint16_t userId, const QString &details)
{
  logEvent(SecurityEvent::UNAUTHORIZED_ACCESS, userId, AuthResult::NOT_ADMIN, details);
}

bool AuditLogger::verifyIntegrity() const
{
  QVector<AuditEntry> entries = readAllEntries();

  if (entries.isEmpty())
  {
    return true; // Empty log is valid
  }

  std::array<uint8_t, HASH_SIZE> prevHash{};
  prevHash.fill(0); // Genesis hash

  for (const auto &entry : entries)
  {
    // Verify previous hash matches
    if (entry.prevHash != prevHash)
    {
      qCritical() << "Hash chain broken! Entry timestamp:" << entry.timestamp;
      return false;
    }

    // Recompute entry hash
    QByteArray computedHash = computeEntryHash(entry);

    // Verify stored hash matches
    if (!std::equal(computedHash.begin(), computedHash.end(), entry.entryHash.begin()))
    {
      qCritical() << "Entry hash mismatch! Entry timestamp:" << entry.timestamp;
      return false;
    }

    prevHash = entry.entryHash;
  }

  qInfo() << "Audit log integrity verified. Entries:" << entries.size();
  return true;
}

QStringList AuditLogger::getRecentEntries(int count) const
{
  QVector<AuditEntry> entries = readAllEntries();
  QStringList result;

  // Get last 'count' entries
  int start = std::max(0, static_cast<int>(entries.size()) - count);
  for (int i = start; i < entries.size(); i++)
  {
    result.append(formatEntry(entries[i]));
  }

  return result;
}

bool AuditLogger::exportLog(const QString &outputPath) const
{
  QStringList allEntries = getRecentEntries(10000); // Export all entries

  QFile file(outputPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    qCritical() << "Failed to export log:" << file.errorString();
    return false;
  }

  QTextStream out(&file);
  out << "CARBIO Security Audit Log Export\n";
  out << "Export Date: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
  out << "Total Entries: " << allEntries.size() << "\n";
  out << QString("-").repeated(80) << "\n\n";

  for (const QString &entry : allEntries)
  {
    out << entry << "\n";
  }

  file.close();
  qInfo() << "Audit log exported to:" << outputPath;
  return true;
}

void AuditLogger::initialize()
{
  loadEncryptionKey();

  // Load existing log to get last hash
  QVector<AuditEntry> entries = readAllEntries();
  if (!entries.isEmpty())
  {
    m_lastHash = entries.last().entryHash;
    m_entryCount = entries.size();
    qInfo() << "Loaded existing audit log with" << m_entryCount << "entries";
  }
  else
  {
    // Initialize with genesis hash
    m_lastHash.fill(0);
    m_entryCount = 0;
    qInfo() << "Initialized new audit log";
  }
}

void AuditLogger::loadEncryptionKey()
{
  QString keyPath = getKeyFilePath();
  QFile file(keyPath);

  if (!file.exists())
  {
    qInfo() << "No encryption key found. Generating new key...";

    // Generate new encryption key
    auto *rng = QRandomGenerator::system();
    m_encryptionKey.resize(static_cast<int>(ENCRYPTION_KEY_SIZE));

    for (size_t i = 0; i < ENCRYPTION_KEY_SIZE; i++)
    {
      m_encryptionKey[static_cast<int>(i)] = static_cast<char>(rng->generate() & 0xFF);
    }

    saveEncryptionKey();
    return;
  }

  if (!file.open(QIODevice::ReadOnly))
  {
    qCritical() << "Failed to load encryption key:" << file.errorString();
    return;
  }

  m_encryptionKey = file.readAll();
  file.close();

  if (m_encryptionKey.size() != ENCRYPTION_KEY_SIZE)
  {
    qWarning() << "Invalid encryption key size. Regenerating...";
    m_encryptionKey.clear();
    loadEncryptionKey();
    return;
  }

  qInfo() << "Encryption key loaded successfully";
}

void AuditLogger::saveEncryptionKey()
{
  QString keyPath = getKeyFilePath();

  // Ensure directory exists
  QFileInfo fileInfo(keyPath);
  QDir dir = fileInfo.dir();
  if (!dir.exists())
  {
    dir.mkpath(".");
  }

  QFile file(keyPath);
  if (!file.open(QIODevice::WriteOnly))
  {
    qCritical() << "Failed to save encryption key:" << file.errorString();
    return;
  }

  file.write(m_encryptionKey);
  file.close();

  // Set restrictive permissions
  file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

  qInfo() << "Encryption key saved to:" << keyPath;
}

QByteArray AuditLogger::computeEntryHash(const AuditEntry &entry) const
{
  QCryptographicHash hash(QCryptographicHash::Sha256);

  // Hash all entry fields
  hash.addData(QByteArrayView(reinterpret_cast<const char *>(&entry.timestamp), sizeof(entry.timestamp)));
  hash.addData(QByteArrayView(reinterpret_cast<const char *>(&entry.event), sizeof(entry.event)));
  hash.addData(QByteArrayView(reinterpret_cast<const char *>(&entry.userId), sizeof(entry.userId)));
  hash.addData(QByteArrayView(reinterpret_cast<const char *>(&entry.result), sizeof(entry.result)));
  hash.addData(QByteArrayView(entry.ipAddress.c_str(), static_cast<qsizetype>(entry.ipAddress.size())));
  hash.addData(QByteArrayView(entry.details.c_str(), static_cast<qsizetype>(entry.details.size())));
  hash.addData(QByteArrayView(reinterpret_cast<const char *>(entry.prevHash.data()), HASH_SIZE));

  return hash.result();
}

QByteArray AuditLogger::encryptEntry(const QByteArray &plaintext) const
{
  // For simplicity, using XOR with key stream (in production, use Qt's AES if available or OpenSSL)
  // Note: Qt doesn't have built-in AES, so this is a simplified version
  // In production, integrate with OpenSSL or use Qt Cryptographic Architecture

  // Generate random IV
  auto *rng = QRandomGenerator::system();
  QByteArray iv(static_cast<int>(IV_SIZE), 0);
  for (size_t i = 0; i < IV_SIZE; i++)
  {
    iv[static_cast<int>(i)] = static_cast<char>(rng->generate() & 0xFF);
  }

  // Simple stream cipher (XOR with key-derived stream)
  // In production: Use AES-256-CBC
  QByteArray ciphertext = plaintext;
  for (int i = 0; i < ciphertext.size(); i++)
  {
    int keyIndex = static_cast<int>(static_cast<qsizetype>(i) % m_encryptionKey.size());
    int ivIndex = static_cast<int>(static_cast<size_t>(i) % IV_SIZE);
    ciphertext[i] = static_cast<char>(ciphertext[i] ^ m_encryptionKey[keyIndex] ^ iv[ivIndex]);
  }

  // Prepend IV
  return iv + ciphertext;
}

QByteArray AuditLogger::decryptEntry(const QByteArray &ciphertext) const
{
  if (ciphertext.size() < static_cast<qsizetype>(IV_SIZE))
  {
    return QByteArray();
  }

  QByteArray iv = ciphertext.left(static_cast<int>(IV_SIZE));
  QByteArray encrypted = ciphertext.mid(static_cast<int>(IV_SIZE));

  // Decrypt using same XOR stream
  QByteArray plaintext = encrypted;
  for (int i = 0; i < plaintext.size(); i++)
  {
    int keyIndex = static_cast<int>(static_cast<qsizetype>(i) % m_encryptionKey.size());
    int ivIndex = static_cast<int>(static_cast<size_t>(i) % IV_SIZE);
    plaintext[i] = static_cast<char>(plaintext[i] ^ m_encryptionKey[keyIndex] ^ iv[ivIndex]);
  }

  return plaintext;
}

void AuditLogger::appendToLog(const AuditEntry &entry)
{
  QString logPath = getLogFilePath();

  // Ensure directory exists
  QFileInfo fileInfo(logPath);
  QDir dir = fileInfo.dir();
  if (!dir.exists())
  {
    dir.mkpath(".");
  }

  // Serialize entry
  QByteArray entryData;
  entryData.append(reinterpret_cast<const char *>(&entry.timestamp), sizeof(entry.timestamp));
  entryData.append(reinterpret_cast<const char *>(&entry.event), sizeof(entry.event));
  entryData.append(reinterpret_cast<const char *>(&entry.userId), sizeof(entry.userId));
  entryData.append(reinterpret_cast<const char *>(&entry.result), sizeof(entry.result));

  auto ipLen = static_cast<uint32_t>(entry.ipAddress.size());
  auto detailsLen = static_cast<uint32_t>(entry.details.size());
  entryData.append(reinterpret_cast<const char *>(&ipLen), sizeof(ipLen));
  entryData.append(entry.ipAddress.c_str(), ipLen);
  entryData.append(reinterpret_cast<const char *>(&detailsLen), sizeof(detailsLen));
  entryData.append(entry.details.c_str(), detailsLen);

  entryData.append(reinterpret_cast<const char *>(entry.prevHash.data()), HASH_SIZE);
  entryData.append(reinterpret_cast<const char *>(entry.entryHash.data()), HASH_SIZE);

  // Encrypt entry
  QByteArray encrypted = encryptEntry(entryData);

  // Write to file (append mode)
  QFile file(logPath);
  if (!file.open(QIODevice::Append))
  {
    qCritical() << "Failed to append to log file:" << file.errorString();
    return;
  }

  // Write entry size + encrypted data
  auto entrySize = static_cast<uint32_t>(encrypted.size());
  file.write(reinterpret_cast<const char *>(&entrySize), sizeof(entrySize));
  file.write(encrypted);
  file.close();

  // Set append-only permissions (where supported)
  file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
}

QVector<AuditEntry> AuditLogger::readAllEntries() const
{
  QString logPath = getLogFilePath();
  QFile file(logPath);

  QVector<AuditEntry> entries;

  if (!file.exists())
  {
    return entries;
  }

  if (!file.open(QIODevice::ReadOnly))
  {
    qWarning() << "Failed to read log file:" << file.errorString();
    return entries;
  }

  while (!file.atEnd())
  {
    // Read entry size
    uint32_t entrySize;
    if (file.read(reinterpret_cast<char *>(&entrySize), sizeof(entrySize)) != sizeof(entrySize))
    {
      break;
    }

    // Read encrypted entry
    QByteArray encrypted = file.read(entrySize);
    if (encrypted.size() != static_cast<int>(entrySize))
    {
      qWarning() << "Incomplete entry read";
      break;
    }

    // Decrypt
    QByteArray entryData = decryptEntry(encrypted);

    // Deserialize
    AuditEntry entry;
    int offset = 0;

    std::memcpy(&entry.timestamp, entryData.constData() + offset, sizeof(entry.timestamp));
    offset += sizeof(entry.timestamp);

    std::memcpy(&entry.event, entryData.constData() + offset, sizeof(entry.event));
    offset += sizeof(entry.event);

    std::memcpy(&entry.userId, entryData.constData() + offset, sizeof(entry.userId));
    offset += sizeof(entry.userId);

    std::memcpy(&entry.result, entryData.constData() + offset, sizeof(entry.result));
    offset += sizeof(entry.result);

    uint32_t ipLen, detailsLen;
    std::memcpy(&ipLen, entryData.constData() + offset, sizeof(ipLen));
    offset += sizeof(ipLen);

    entry.ipAddress = std::string(entryData.constData() + offset, ipLen);
    offset += ipLen;

    std::memcpy(&detailsLen, entryData.constData() + offset, sizeof(detailsLen));
    offset += sizeof(detailsLen);

    entry.details = std::string(entryData.constData() + offset, detailsLen);
    offset += detailsLen;

    std::memcpy(entry.prevHash.data(), entryData.constData() + offset, HASH_SIZE);
    offset += HASH_SIZE;

    std::memcpy(entry.entryHash.data(), entryData.constData() + offset, HASH_SIZE);

    entries.append(entry);
  }

  file.close();
  return entries;
}

QString AuditLogger::getLogFilePath() const
{
  QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return dataDir + "/carbio_audit.log";
}

QString AuditLogger::getKeyFilePath() const
{
  QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  return configDir + "/carbio_audit.key";
}

QString AuditLogger::formatEntry(const AuditEntry &entry) const
{
  QDateTime dt = QDateTime::fromSecsSinceEpoch(entry.timestamp);

  return QString("[%1] %2 | User: %3 | Result: %4 | %5")
      .arg(dt.toString(Qt::ISODate))
      .arg(QString::fromStdString(std::string(securityEventToString(entry.event))))
      .arg(entry.userId)
      .arg(QString::fromStdString(std::string(authResultToString(entry.result))))
      .arg(QString::fromStdString(entry.details));
}
