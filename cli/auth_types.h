#ifndef AUTH_TYPES_H
#define AUTH_TYPES_H

#include <cstdint>
#include <string_view>

// Byte-sized enum for authentication state machine
enum class AuthState : uint8_t
{
  OFF            = 0,
  SCANNING       = 1,
  AUTHENTICATING = 2,
  ALERT          = 3,
  ON             = 4
};

// Compile-time string conversion for AuthState
[[nodiscard]] inline constexpr std::string_view authStateToString(AuthState state) noexcept
{
  switch (state)
  {
  case AuthState::OFF:
    return "OFF";
  case AuthState::SCANNING:
    return "SCANNING";
  case AuthState::AUTHENTICATING:
    return "AUTHENTICATING";
  case AuthState::ALERT:
    return "ALERT";
  case AuthState::ON:
    return "ON";
  default:
    return "UNKNOWN";
  }
}

// Authentication result
enum class AuthResult : uint8_t
{
  Success,
  Failed,
  Cancelled,
  Timeout
};

#endif // AUTH_TYPES_H
