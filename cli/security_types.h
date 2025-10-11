#ifndef SECURITY_TYPES_H
#define SECURITY_TYPES_H

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace carbio::security
{
enum class SecurityEvent : uint8_t
{
  ADMIN_ACCESS_ATTEMPT = 0,
  ADMIN_ACCESS_GRANTED = 1,
  ADMIN_ACCESS_DENIED  = 2,
  PASSWORD_VERIFIED    = 3,
  PASSWORD_FAILED      = 4,
  FINGERPRINT_VERIFIED = 5,
  FINGERPRINT_FAILED   = 6,
  TOKEN_GENERATED      = 7,
  TOKEN_VALIDATED      = 8,
  TOKEN_EXPIRED        = 9,
  TOKEN_INVALID        = 10,
  UNAUTHORIZED_ACCESS  = 11,
  RATE_LIMIT_EXCEEDED  = 12,
  SESSION_STARTED      = 13,
  SESSION_ENDED        = 14
};

enum class AuthResult : uint8_t
{
  SUCCESS              = 0,
  INVALID_PASSWORD     = 1,
  INVALID_FINGERPRINT  = 2,
  NOT_ADMIN            = 3,
  RATE_LIMITED         = 4,
  TOKEN_EXPIRED        = 5,
  TOKEN_INVALID        = 6,
  SYSTEM_ERROR         = 7
};

// Admin privilege levels (fingerprint IDs 0-2 are admins)
constexpr uint16_t ADMIN_ID_MIN = 0;
constexpr uint16_t ADMIN_ID_MAX = 2;
constexpr uint16_t MIN_ADMIN_CONFIDENCE = 150;

// Security timing constants
constexpr int CHALLENGE_WINDOW_SECONDS = 30;
constexpr int TOKEN_LIFETIME_SECONDS = 300; // 5 minutes
constexpr int MAX_AUTH_ATTEMPTS = 3;
constexpr int RATE_LIMIT_WINDOW_SECONDS = 30;
constexpr int LOCKOUT_DURATION_SECONDS = 300; // 5 minutes after max attempts

// Cryptographic constants
constexpr size_t SALT_SIZE = 16;
constexpr size_t HASH_SIZE = 32;
constexpr size_t NONCE_SIZE = 16;
constexpr size_t TOKEN_SIZE = 32;
constexpr size_t HMAC_SIZE = 32;

// Session token structure (cryptographically signed)
struct SessionToken
{
  std::array<uint8_t, TOKEN_SIZE> token;
  std::array<uint8_t, HMAC_SIZE>  signature;
  uint64_t                        timestamp;
  uint16_t                        adminId;
  bool                            used;

  SessionToken()
    : token{}, signature{}, timestamp(0), adminId(0), used(false)
  {
  }
};

// Audit log entry structure
struct AuditEntry
{
  uint64_t      timestamp;
  SecurityEvent event;
  uint16_t      userId;
  AuthResult    result;
  std::string   ipAddress;
  std::string   details;
  std::array<uint8_t, HASH_SIZE> prevHash;
  std::array<uint8_t, HASH_SIZE> entryHash;

  // Explicitly define copy/move/destructor as noinline (struct too large to inline)
  [[gnu::noinline]] AuditEntry(const AuditEntry&) = default;
  AuditEntry& operator=(const AuditEntry&) = default;
  AuditEntry(AuditEntry&&) noexcept = default;
  AuditEntry& operator=(AuditEntry&&) noexcept = default;
  AuditEntry() = default;
  [[gnu::noinline]] ~AuditEntry() = default;
};

// Convert enums to strings for logging
[[nodiscard]] inline constexpr std::string_view securityEventToString(SecurityEvent event) noexcept
{
  switch (event)
  {
  case SecurityEvent::ADMIN_ACCESS_ATTEMPT:
    return "ADMIN_ACCESS_ATTEMPT";
  case SecurityEvent::ADMIN_ACCESS_GRANTED:
    return "ADMIN_ACCESS_GRANTED";
  case SecurityEvent::ADMIN_ACCESS_DENIED:
    return "ADMIN_ACCESS_DENIED";
  case SecurityEvent::PASSWORD_VERIFIED:
    return "PASSWORD_VERIFIED";
  case SecurityEvent::PASSWORD_FAILED:
    return "PASSWORD_FAILED";
  case SecurityEvent::FINGERPRINT_VERIFIED:
    return "FINGERPRINT_VERIFIED";
  case SecurityEvent::FINGERPRINT_FAILED:
    return "FINGERPRINT_FAILED";
  case SecurityEvent::TOKEN_GENERATED:
    return "TOKEN_GENERATED";
  case SecurityEvent::TOKEN_VALIDATED:
    return "TOKEN_VALIDATED";
  case SecurityEvent::TOKEN_EXPIRED:
    return "TOKEN_EXPIRED";
  case SecurityEvent::TOKEN_INVALID:
    return "TOKEN_INVALID";
  case SecurityEvent::UNAUTHORIZED_ACCESS:
    return "UNAUTHORIZED_ACCESS";
  case SecurityEvent::RATE_LIMIT_EXCEEDED:
    return "RATE_LIMIT_EXCEEDED";
  case SecurityEvent::SESSION_STARTED:
    return "SESSION_STARTED";
  case SecurityEvent::SESSION_ENDED:
    return "SESSION_ENDED";
  default:
    return "UNKNOWN";
  }
}

[[nodiscard]] inline constexpr std::string_view authResultToString(AuthResult result) noexcept
{
  switch (result)
  {
  case AuthResult::SUCCESS:
    return "SUCCESS";
  case AuthResult::INVALID_PASSWORD:
    return "INVALID_PASSWORD";
  case AuthResult::INVALID_FINGERPRINT:
    return "INVALID_FINGERPRINT";
  case AuthResult::NOT_ADMIN:
    return "NOT_ADMIN";
  case AuthResult::RATE_LIMITED:
    return "RATE_LIMITED";
  case AuthResult::TOKEN_EXPIRED:
    return "TOKEN_EXPIRED";
  case AuthResult::TOKEN_INVALID:
    return "TOKEN_INVALID";
  case AuthResult::SYSTEM_ERROR:
    return "SYSTEM_ERROR";
  default:
    return "UNKNOWN";
  }
}

} // namespace carbio::security

#endif // SECURITY_TYPES_H
