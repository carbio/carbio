#include "fingerprint/user_data.h"
#include "utility/endian.h"

namespace carbio
{
std::array<std::uint8_t, 32> user_data::to_byte_stream() noexcept
{
  std::array<std::uint8_t, 32> result{};
  write_le<std::uint32_t>(std::span(result).subspan(0, 4), enrolled_at);
  write_le<std::uint32_t>(std::span(result).subspan(4, 4), last_login_at);
  result[8] = id;
  result[9] = static_cast<std::uint8_t>(role);
  result[10] = static_cast<std::uint8_t>(status);
  std::memcpy(result.data() + 11, name.data(), name.size());
  result[31] = 0;
  return result;
}

user_data& user_data::from_byte_stream(std::array<std::uint8_t, 32> data) noexcept
{
  enrolled_at = read_le<std::uint32_t>(std::span(data).subspan(0, 4));
  last_login_at = read_le<std::uint32_t>(std::span(data).subspan(4, 4));
  id = data[8];
  role = static_cast<role_type>(data[9]);
  status = static_cast<role_status>(data[10]);
  std::memcpy(name.data(), data.data() + 11, name.size());
  name[name.size()-1] = 0;
  return *this;
}
}