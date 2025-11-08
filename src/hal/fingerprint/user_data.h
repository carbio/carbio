#pragma once

#include <array>
#include <cstdint>

namespace carbio
{
enum class role_type : std::uint8_t
{
  primary_owner,
  secondary_owner,
  ternary_owner,
  restricted,
  guest,
  service
};

enum class role_status : std::uint8_t
{
  unknown,
  pending,
  active,
  suspended,
  expired,
  revoked
};

struct user_data
{
  std::uint32_t enrolled_at;
  std::uint32_t last_login_at;
  std::uint8_t id;
  role_type role;
  role_status status;
  std::array<char, 21> name;
  std::array<std::uint8_t, 32> to_byte_stream() noexcept;
  user_data& from_byte_stream(std::array<std::uint8_t, 32> data) noexcept;
};
} // namespace carbio
