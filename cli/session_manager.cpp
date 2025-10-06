#include "session_manager.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

using namespace carbio::security;
using namespace std::chrono;

SessionManager::SessionManager(QObject *parent)
  : QObject(parent)
  , m_cleanupTimer(new QTimer(this))
{
  // Load or generate secret key
  loadSecretKey();

  // Setup periodic cleanup
  connect(m_cleanupTimer, &QTimer::timeout, this, &SessionManager::cleanupExpiredTokens);
  m_cleanupTimer->start(CLEANUP_INTERVAL_MS);
}

SessionManager::~SessionManager()
{
  // Clear sensitive data
  m_secretKey.fill(0);
  for (auto &session : m_activeSessions)
  {
    if (session.has_value())
    {
      session->token.fill(0);
      session->signature.fill(0);
    }
  }
}

QString SessionManager::generateToken(uint16_t adminId)
{
  if (adminId > ADMIN_ID_MAX)
  {
    qWarning() << "Invalid admin ID:" << adminId;
    return QString();
  }

  // Generate cryptographically secure random token
  SessionToken token;
  auto *rng = QRandomGenerator::system();

  for (size_t i = 0; i < TOKEN_SIZE; i++)
  {
    token.token[i] = static_cast<uint8_t>(rng->generate() & 0xFF);
  }

  // Set metadata
  token.timestamp = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  token.adminId = adminId;
  token.used = false;

  // Generate HMAC signature
  QByteArray tokenBytes(reinterpret_cast<const char *>(token.token.data()), TOKEN_SIZE);
  QByteArray signature = generateHMAC(tokenBytes, token.timestamp, adminId);

  std::copy_n(signature.constData(), HMAC_SIZE, token.signature.begin());

  // Store session
  m_activeSessions[adminId] = token;

  // Encode token + signature + timestamp + adminId as Base64 for transmission
  QByteArray payload;
  payload.append(reinterpret_cast<const char *>(token.token.data()), TOKEN_SIZE);
  payload.append(reinterpret_cast<const char *>(token.signature.data()), HMAC_SIZE);
  payload.append(reinterpret_cast<const char *>(&token.timestamp), sizeof(token.timestamp));
  payload.append(reinterpret_cast<const char *>(&token.adminId), sizeof(token.adminId));

  QString encoded = QString::fromLatin1(payload.toBase64());

  qInfo() << "Session token generated for admin ID:" << adminId;
  emit tokenGenerated(adminId);

  return encoded;
}

AuthResult SessionManager::validateToken(const QString &tokenString, uint16_t adminId)
{
  if (adminId > ADMIN_ID_MAX)
  {
    qWarning() << "Invalid admin ID for validation:" << adminId;
    return AuthResult::NOT_ADMIN;
  }

  // Decode Base64 token
  QByteArray payload = QByteArray::fromBase64(tokenString.toLatin1());

  // Validate payload size
  size_t expectedSize = TOKEN_SIZE + HMAC_SIZE + sizeof(uint64_t) + sizeof(uint16_t);
  if (payload.size() != static_cast<int>(expectedSize))
  {
    qWarning() << "Invalid token size. Expected:" << expectedSize << "Got:" << payload.size();
    return AuthResult::TOKEN_INVALID;
  }

  // Extract components
  QByteArray tokenBytes = payload.left(TOKEN_SIZE);
  QByteArray signature = payload.mid(TOKEN_SIZE, HMAC_SIZE);
  uint64_t timestamp;
  uint16_t tokenAdminId;

  std::memcpy(&timestamp, payload.constData() + TOKEN_SIZE + HMAC_SIZE, sizeof(timestamp));
  std::memcpy(&tokenAdminId, payload.constData() + TOKEN_SIZE + HMAC_SIZE + sizeof(timestamp), sizeof(tokenAdminId));

  // Verify admin ID matches
  if (tokenAdminId != adminId)
  {
    qWarning() << "Admin ID mismatch. Token:" << tokenAdminId << "Expected:" << adminId;
    return AuthResult::NOT_ADMIN;
  }

  // Verify HMAC signature
  if (!verifyHMAC(tokenBytes, timestamp, adminId, signature))
  {
    qWarning() << "HMAC verification failed - token may be forged";
    return AuthResult::TOKEN_INVALID;
  }

  // Check expiry
  uint64_t currentTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  if (currentTime > timestamp + TOKEN_LIFETIME_SECONDS)
  {
    qWarning() << "Token expired. Age:" << (currentTime - timestamp) << "seconds";
    emit tokenExpired(adminId);
    return AuthResult::TOKEN_EXPIRED;
  }

  // Check if token exists in active sessions
  if (!m_activeSessions[adminId].has_value())
  {
    qWarning() << "No active session for admin ID:" << adminId;
    return AuthResult::TOKEN_INVALID;
  }

  // Verify token matches stored token
  QByteArray storedTokenBytes(reinterpret_cast<const char *>(m_activeSessions[adminId]->token.data()), TOKEN_SIZE);
  if (!constantTimeCompare(tokenBytes, storedTokenBytes))
  {
    qWarning() << "Token mismatch - possible replay attack";
    return AuthResult::TOKEN_INVALID;
  }

  // Check if already used (single-use token)
  if (m_activeSessions[adminId]->used)
  {
    qWarning() << "Token already used - replay attack detected";
    return AuthResult::TOKEN_INVALID;
  }

  // Mark as used
  m_activeSessions[adminId]->used = true;

  qInfo() << "Token validated successfully for admin ID:" << adminId;
  emit tokenValidated(adminId);

  return AuthResult::SUCCESS;
}

bool SessionManager::hasValidSession(uint16_t adminId) const
{
  if (adminId > ADMIN_ID_MAX || !m_activeSessions[adminId].has_value())
  {
    return false;
  }

  const auto &session = m_activeSessions[adminId].value();

  // Check if expired
  uint64_t currentTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  if (currentTime > session.timestamp + TOKEN_LIFETIME_SECONDS)
  {
    return false;
  }

  // Check if already used
  if (session.used)
  {
    return false;
  }

  return true;
}

void SessionManager::revokeAllSessions()
{
  qWarning() << "Revoking all active sessions";

  for (size_t i = 0; i <= ADMIN_ID_MAX; i++)
  {
    if (m_activeSessions[i].has_value())
    {
      m_activeSessions[i]->token.fill(0);
      m_activeSessions[i]->signature.fill(0);
      m_activeSessions[i].reset();
      emit tokenRevoked(static_cast<uint16_t>(i));
    }
  }
}

void SessionManager::revokeSession(uint16_t adminId)
{
  if (adminId > ADMIN_ID_MAX)
  {
    return;
  }

  if (m_activeSessions[adminId].has_value())
  {
    m_activeSessions[adminId]->token.fill(0);
    m_activeSessions[adminId]->signature.fill(0);
    m_activeSessions[adminId].reset();

    qInfo() << "Session revoked for admin ID:" << adminId;
    emit tokenRevoked(adminId);
  }
}

int SessionManager::sessionTimeRemaining(uint16_t adminId) const
{
  if (!hasValidSession(adminId))
  {
    return 0;
  }

  const auto &session = m_activeSessions[adminId].value();
  uint64_t currentTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  int64_t remaining = static_cast<int64_t>(session.timestamp + TOKEN_LIFETIME_SECONDS - currentTime);

  return static_cast<int>(std::max(0L, remaining));
}

QByteArray SessionManager::generateHMAC(const QByteArray &token, uint64_t timestamp, uint16_t adminId) const
{
  QMessageAuthenticationCode hmac(QCryptographicHash::Sha256, m_secretKey);

  hmac.addData(token);
  hmac.addData(reinterpret_cast<const char *>(&timestamp), sizeof(timestamp));
  hmac.addData(reinterpret_cast<const char *>(&adminId), sizeof(adminId));

  return hmac.result();
}

bool SessionManager::verifyHMAC(const QByteArray &token, uint64_t timestamp, uint16_t adminId,
                                 const QByteArray &signature) const
{
  QByteArray expectedSignature = generateHMAC(token, timestamp, adminId);
  return constantTimeCompare(signature, expectedSignature);
}

void SessionManager::cleanupExpiredTokens()
{
  uint64_t currentTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

  for (size_t i = 0; i <= ADMIN_ID_MAX; i++)
  {
    if (m_activeSessions[i].has_value())
    {
      const auto &session = m_activeSessions[i].value();
      uint64_t expiryTime = session.timestamp + TOKEN_LIFETIME_SECONDS;

      if (currentTime > expiryTime)
      {
        qInfo() << "Cleaning up expired session for admin ID:" << i;
        m_activeSessions[i].reset();
        emit tokenExpired(static_cast<uint16_t>(i));
      }
      else if (currentTime > expiryTime - EXPIRY_WARNING_SECONDS)
      {
        int remaining = static_cast<int>(expiryTime - currentTime);
        emit sessionExpiring(static_cast<uint16_t>(i), remaining);
      }
    }
  }
}

void SessionManager::loadSecretKey()
{
  QString keyPath = getSecretKeyPath();
  QFile file(keyPath);

  if (!file.exists())
  {
    qInfo() << "No secret key found. Generating new key...";

    // Generate new secret key
    auto *rng = QRandomGenerator::system();
    m_secretKey.resize(static_cast<int>(SECRET_KEY_SIZE));

    for (size_t i = 0; i < SECRET_KEY_SIZE; i++)
    {
      m_secretKey[static_cast<int>(i)] = static_cast<char>(rng->generate() & 0xFF);
    }

    saveSecretKey();
    return;
  }

  if (!file.open(QIODevice::ReadOnly))
  {
    qCritical() << "Failed to load secret key:" << file.errorString();
    return;
  }

  m_secretKey = file.readAll();
  file.close();

  if (m_secretKey.size() != SECRET_KEY_SIZE)
  {
    qWarning() << "Invalid secret key size. Generating new key...";
    m_secretKey.clear();
    loadSecretKey(); // Regenerate
    return;
  }

  qInfo() << "Secret key loaded successfully";
}

void SessionManager::saveSecretKey()
{
  QString keyPath = getSecretKeyPath();

  // Ensure directory exists
  QFileInfo fileInfo(keyPath);
  QDir dir = fileInfo.dir();
  if (!dir.exists())
  {
    if (!dir.mkpath("."))
    {
      qCritical() << "Failed to create directory:" << dir.path();
      return;
    }
  }

  QFile file(keyPath);
  if (!file.open(QIODevice::WriteOnly))
  {
    qCritical() << "Failed to save secret key:" << file.errorString();
    return;
  }

  file.write(m_secretKey);
  file.close();

  // Set restrictive permissions (owner read/write only)
  file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

  qInfo() << "Secret key saved to:" << keyPath;
}

QString SessionManager::getSecretKeyPath() const
{
  QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  return configDir + "/carbio_session.key";
}

bool SessionManager::constantTimeCompare(const QByteArray &a, const QByteArray &b) const
{
  if (a.size() != b.size())
  {
    return false;
  }

  uint8_t diff = 0;
  for (int i = 0; i < a.size(); i++)
  {
    diff = static_cast<uint8_t>(diff | static_cast<uint8_t>(a[i] ^ b[i]));
  }

  return diff == 0;
}
