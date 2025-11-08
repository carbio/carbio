#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace carbio
{
template<std::integral T>
std::optional<T> parse_integral(std::string_view s)
{
  T result;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);
  if (ec == std::errc{} && ptr == s.data() + s.size()) return result;
  return std::nullopt;
}
} // namespace carbio