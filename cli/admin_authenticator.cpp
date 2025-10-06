#include "admin_authenticator.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QStandardPaths>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPasswordDigestor>
#endif

#include <algorithm>
#include <cstring>

using namespace carbio::security;
using namespace std::chrono;

AdminAuthenticator::AdminAuthenticator(QObject *parent)
  : QObject(parent)
  , m_failedAttempts(0)
  , m_lockoutUntil(steady_clock::time_point::min())
  , m_lastAttempt(steady_clock::time_point::min())
  , m_activeChallenge{std::array<uint8_t, NONCE_SIZE>{}, steady_clock::time_point{}, false}
{
  loadPasswordHash();

  // If no password is set, use default (with warning)
  if (!hasPasswordSet())
  {
    qWarning() << "No admin password set! Using default password. CHANGE THIS IMMEDIATELY!";
    static_cast<void>(setPassword(DEFAULT_PASSWORD));
  }
}

AdminAuthenticator::~AdminAuthenticator()
{
  // Clear sensitive data from memory
  m_passwordHash.fill(0);
  m_salt.fill(0);
  m_activeChallenge.nonce.fill(0);
}

bool AdminAuthenticator::verifyPassword(const QString &password)
{
  // Check rate limiting
  if (isRateLimited())
  {
    qWarning() << "Rate limited - too many failed attempts";
    emit rateLimitTriggered(lockoutSecondsRemaining());
    return false;
  }

  // Update last attempt timestamp
  m_lastAttempt = steady_clock::now();

  // Validate password
  QByteArray computedHash = hashPassword(password, m_salt);
  bool valid = constantTimeCompare(computedHash, m_passwordHash);

  if (valid)
  {
    qInfo() << "Password verified successfully";
    m_failedAttempts = 0;
    emit passwordVerified();
    return true;
  }
  else
  {
    m_failedAttempts++;
    qWarning() << "Password verification failed. Attempts:" << m_failedAttempts << "/" << MAX_ATTEMPTS;

    if (m_failedAttempts >= MAX_ATTEMPTS)
    {
      m_lockoutUntil = steady_clock::now() + seconds(LOCKOUT_DURATION_SECONDS);
      qWarning() << "Max attempts reached. Locked out for" << LOCKOUT_DURATION_SECONDS << "seconds";
      emit rateLimitTriggered(LOCKOUT_DURATION_SECONDS);
    }
    else
    {
      emit passwordFailed(MAX_ATTEMPTS - m_failedAttempts);
    }

    // Clear sensitive data
    computedHash.fill(0);
    return false;
  }
}

bool AdminAuthenticator::setPassword(const QString &newPassword)
{
  if (newPassword.length() < static_cast<int>(PASSWORD_MIN_LENGTH))
  {
    qWarning() << "Password too short. Minimum length:" << PASSWORD_MIN_LENGTH;
    return false;
  }

  // Generate new salt
  m_salt = generateSalt();

  // Hash the password
  m_passwordHash = hashPassword(newPassword, m_salt);

  // Save to persistent storage
  savePasswordHash();

  qInfo() << "Admin password updated successfully";
  return true;
}

bool AdminAuthenticator::hasPasswordSet() const
{
  return !m_passwordHash.isEmpty() && !m_salt.isEmpty();
}

QByteArray AdminAuthenticator::generateChallenge()
{
  // Generate cryptographically secure random nonce
  auto *rng = QRandomGenerator::system();
  QByteArray nonce(static_cast<int>(NONCE_SIZE), 0);

  for (size_t i = 0; i < NONCE_SIZE; i++)
  {
    nonce[static_cast<int>(i)] = static_cast<char>(rng->generate() & 0xFF);
  }

  // Store challenge
  std::copy_n(nonce.constData(), NONCE_SIZE, m_activeChallenge.nonce.begin());
  m_activeChallenge.timestamp = steady_clock::now();
  m_activeChallenge.active = true;

  qDebug() << "Challenge generated (valid for" << CHALLENGE_WINDOW_SECONDS << "seconds)";
  return nonce;
}

bool AdminAuthenticator::validateChallenge(const QByteArray &nonce)
{
  if (!m_activeChallenge.active)
  {
    qWarning() << "No active challenge to validate";
    return false;
  }

  // Check expiry
  auto elapsed = duration_cast<seconds>(steady_clock::now() - m_activeChallenge.timestamp).count();
  if (elapsed > CHALLENGE_WINDOW_SECONDS)
  {
    qWarning() << "Challenge expired (" << elapsed << "s elapsed)";
    clearChallenge();
    return false;
  }

  // Validate nonce (constant-time comparison)
  if (nonce.size() != NONCE_SIZE)
  {
    qWarning() << "Invalid nonce size";
    return false;
  }

  QByteArray storedNonce(reinterpret_cast<const char *>(m_activeChallenge.nonce.data()), NONCE_SIZE);
  bool valid = constantTimeCompare(nonce, storedNonce);

  if (valid)
  {
    qInfo() << "Challenge validated successfully";
    clearChallenge(); // Single-use challenge
    return true;
  }
  else
  {
    qWarning() << "Challenge validation failed - nonce mismatch";
    return false;
  }
}

void AdminAuthenticator::clearChallenge()
{
  m_activeChallenge.nonce.fill(0);
  m_activeChallenge.active = false;
}

bool AdminAuthenticator::isRateLimited() const
{
  if (m_failedAttempts < MAX_ATTEMPTS)
  {
    return false;
  }

  return steady_clock::now() < m_lockoutUntil;
}

int AdminAuthenticator::remainingAttempts() const
{
  if (isRateLimited())
  {
    return 0;
  }
  return MAX_ATTEMPTS - m_failedAttempts;
}

int AdminAuthenticator::lockoutSecondsRemaining() const
{
  if (!isRateLimited())
  {
    return 0;
  }

  auto remaining = duration_cast<seconds>(m_lockoutUntil - steady_clock::now()).count();
  return static_cast<int>(std::max(0L, remaining));
}

void AdminAuthenticator::resetAttempts()
{
  m_failedAttempts = 0;
  m_lockoutUntil = steady_clock::time_point::min();
  qInfo() << "Failed attempts reset";
}

QByteArray AdminAuthenticator::hashPassword(const QString &password, const QByteArray &salt) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  // Use Qt's built-in PBKDF2 (available in Qt 6.5+)
  return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,
                                             password.toUtf8(),
                                             salt,
                                             PBKDF2_ITERATIONS,
                                             HASH_SIZE);
#else
  // Fallback for older Qt versions - manual PBKDF2-like implementation
  QByteArray derived = salt;
  QByteArray passwordBytes = password.toUtf8();

  for (int i = 0; i < PBKDF2_ITERATIONS; i++)
  {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(derived);
    hash.addData(passwordBytes);
    hash.addData(salt);
    derived = hash.result();
  }

  return derived;
#endif
}

QByteArray AdminAuthenticator::generateSalt() const
{
  auto *rng = QRandomGenerator::system();
  QByteArray salt(static_cast<int>(SALT_SIZE), 0);

  for (size_t i = 0; i < SALT_SIZE; i++)
  {
    salt[static_cast<int>(i)] = static_cast<char>(rng->generate() & 0xFF);
  }

  return salt;
}

bool AdminAuthenticator::constantTimeCompare(const QByteArray &a, const QByteArray &b) const
{
  if (a.size() != b.size())
  {
    return false;
  }

  // Constant-time comparison to prevent timing attacks
  uint8_t diff = 0;
  for (int i = 0; i < a.size(); i++)
  {
    diff = static_cast<uint8_t>(diff | static_cast<uint8_t>(a[i] ^ b[i]));
  }

  return diff == 0;
}

void AdminAuthenticator::loadPasswordHash()
{
  QString filePath = getPasswordFilePath();
  QFile file(filePath);

  if (!file.exists())
  {
    qInfo() << "No password file found at:" << filePath;
    return;
  }

  if (!file.open(QIODevice::ReadOnly))
  {
    qWarning() << "Failed to open password file:" << file.errorString();
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  // Format: SALT_SIZE bytes of salt + HASH_SIZE bytes of hash
  if (data.size() != SALT_SIZE + HASH_SIZE)
  {
    qWarning() << "Invalid password file format. Expected" << (SALT_SIZE + HASH_SIZE) << "bytes, got" << data.size();
    return;
  }

  m_salt = data.left(SALT_SIZE);
  m_passwordHash = data.mid(SALT_SIZE, HASH_SIZE);

  qInfo() << "Password hash loaded successfully";
}

void AdminAuthenticator::savePasswordHash()
{
  QString filePath = getPasswordFilePath();

  // Ensure directory exists
  QFileInfo fileInfo(filePath);
  QDir dir = fileInfo.dir();
  if (!dir.exists())
  {
    if (!dir.mkpath("."))
    {
      qCritical() << "Failed to create directory:" << dir.path();
      return;
    }
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly))
  {
    qCritical() << "Failed to save password file:" << file.errorString();
    return;
  }

  // Write salt + hash
  file.write(m_salt);
  file.write(m_passwordHash);
  file.close();

  // Set restrictive permissions (owner read/write only)
  file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

  qInfo() << "Password hash saved to:" << filePath;
}

QString AdminAuthenticator::getPasswordFilePath() const
{
  QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  return configDir + "/carbio_admin.pwd";
}
