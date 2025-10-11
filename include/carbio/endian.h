/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#ifndef CARBIO_ENDIAN_H
#define CARBIO_ENDIAN_H

#include "carbio/assert.h"

#include <bit>
#include <concepts>
#include <cstdint>
#include <span>

namespace carbio
{
/*!
 * @brief Performs byte swap on an unsigned integral type.
 * @tparam T any unsigned integral type.
 * @param value The value to transform.
 * @returns The value in reversed byte order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T byteswap(T value) noexcept
{
  if constexpr (sizeof(T) == 1)
  {
    return value; // no-op
  }
  else if constexpr (sizeof(T) == 2)
  {
    return static_cast<T>((value >> 8) | (value << 8));
  }
  else if constexpr (sizeof(T) == 4)
  {
    return __builtin_bswap32(value);
  }
  else if constexpr (sizeof(T) == 8)
  {
    return __builtin_bswap64(value);
  }
  else
  {
    static_assert(sizeof(T) <= 8, "Unsupported integral size for byteswap");
  }
}

/*!
 * @brief Converts a given value to big-endian.
 * @tparam T any unsigned integral type.
 * @param value The value to transform.
 * @returns The value in big endian byte order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T to_big_endian(T value) noexcept
{
  if constexpr (std::endian::native == std::endian::big)
  {
    return value;
  }
  else
  {
    return byteswap(value);
  }
}

/*!
 * @brief Converts a given value to little-endian.
 * @tparam T any unsigned integral type.
 * @param value The value to transform.
 * @returns The value in little-endian byte order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T to_little_endian(T value) noexcept
{
  if constexpr (std::endian::native == std::endian::little)
  {
    return value;
  }
  else
  {
    return byteswap(value);
  }
}

/*!
 * @brief Convert value from big-endian to host order.
 * @tparam T any unsigned integral type.
 * @param value The value to transform.
 * @returns The value in host byte-order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T from_big_endian(T value) noexcept
{
  return to_big_endian(value); // symmetric
}

/*!
 * @brief Convert value from little-endian to host order.
 * @tparam T any unsigned integral type.
 * @param value The value to transform.
 * @returns The value in host byte-order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T from_little_endian(T value) noexcept
{
  return to_little_endian(value); // symmetric
}

/*!
 * @brief Read value from a buffer in big-endian byte order.
 * @tparam T any unsigned integral type.
 * @param buffer A buffer containing at least sizeof(T) bytes.
 * @returns A value read.
 */
template <std::unsigned_integral T>
[[nodiscard]] __attribute__((always_inline)) constexpr T read_be(std::span<const std::uint8_t> buffer) noexcept
{
  ct_panic_msg(buffer.size() >= sizeof(T), "Buffer too small for read_be()");
  T result = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i)
  {
    result = static_cast<T>((result << 8) | buffer[i]);
  }
  return result;
}

/*!
 * @brief Read value from a buffer in little-endian byte order.
 * @tparam T any unsigned integral type.
 * @param buffer A buffer containing at least sizeof(T) bytes.
 * @returns A value read.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr T read_le(std::span<const std::uint8_t> buffer) noexcept
{
  ct_panic_msg(buffer.size() >= sizeof(T), "Buffer too small for read_le()");
  T result = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i)
  {
    result |= static_cast<T>(buffer[i]) << (8 * i);
  }
  return result;
}

/*!
 * @brief Write value into a buffer in big-endian byte order.
 * @tparam T any unsigned integral type.
 * @param buffer A buffer with at least sizeof(T) bytes available.
 * @param value A value to write.
 */
template <std::unsigned_integral T>
constexpr void write_be(std::span<std::uint8_t> buffer, T value) noexcept
{
  ct_panic_msg(buffer.size() >= sizeof(T), "Buffer too small for write_be()");
  for (std::size_t i = 0; i < sizeof(T); ++i)
  {
    buffer[sizeof(T) - 1 - i] = static_cast<std::uint8_t>(value & 0xFF);
    value >>= 8;
  }
}

/*!
 * @brief Write value into a buffer in little-endian byte order.
 * @tparam T any unsigned integral type.
 * @param buffer A buffer with at least sizeof(T) bytes available.
 * @param value A value to write.
 */
template <std::unsigned_integral T>
constexpr void write_le(std::span<std::uint8_t> buffer, T value) noexcept
{
  ct_panic_msg(buffer.size() >= sizeof(T), "Buffer too small for write_le()");
  for (std::size_t i = 0; i < sizeof(T); ++i)
  {
    buffer[i] = static_cast<std::uint8_t>(value & 0xFF);
    value >>= 8;
  }
}

/*!
 * @brief Convert value to big-endian byte array.
 * @tparam T any unsigned integral type.
 * @param value The value to convert.
 * @returns Array of bytes in big-endian order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr std::array<std::uint8_t, sizeof(T)> to_bytes_be(T value) noexcept
{
  // Stack protector guard for small types (sizeof(T) < 8)
  [[maybe_unused]] std::array<std::uint8_t, 8> stack_guard{};

  std::array<std::uint8_t, sizeof(T)> result{};
  write_be<T>(result, value);
  return result;
}

/*!
 * @brief Convert value to little-endian byte array.
 * @tparam T any unsigned integral type.
 * @param value The value to convert.
 * @returns Array of bytes in little-endian order.
 */
template <std::unsigned_integral T>
[[nodiscard]] constexpr std::array<std::uint8_t, sizeof(T)> to_bytes_le(T value) noexcept
{
  // Stack protector guard for small types (sizeof(T) < 8)
  [[maybe_unused]] std::array<std::uint8_t, 8> stack_guard{};

  std::array<std::uint8_t, sizeof(T)> result{};
  write_le<T>(result, value);
  return result;
}
} // namespace carbio

#endif // CARBIO_ENDIAN_H
