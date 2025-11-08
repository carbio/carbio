/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
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

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>

namespace carbio
{
/*!
 * @brief Secure value wrapper that automatically clears memory on destruction.
 *
 * This template wraps sensitive primitive values (e.g., biometric match scores,
 * user indices) and ensures they are securely cleared from memory when destroyed.
 * Uses volatile writes to prevent compiler optimization of the clearing operation.
 *
 * @tparam T Value type (must be trivially copyable)
 */
template <typename T>
class scoped_zero
{
  static_assert(std::is_trivially_copyable_v<T>,
                "scoped_zero requires trivially copyable type");

  T value_;

public:
  /*!
   * @brief Construct with default value.
   */
  scoped_zero() noexcept : value_{}
  {
  }

  /*!
   * @brief Construct with specific value.
   * @param val Initial value
   */
  explicit scoped_zero(T val) noexcept : value_(val)
  {
  }

  /*!
   * @brief Destructor - securely clears the value.
   */
  ~scoped_zero() noexcept
  {
    clear();
  }

  /*!
   * @brief Get the stored value.
   * @return Current value
   */
  [[nodiscard]] T get() const noexcept
  {
    return value_;
  }

  /*!
   * @brief Set a new value.
   * @param val New value
   */
  void set(T val) noexcept
  {
    value_ = val;
  }

  /*!
   * @brief Implicit conversion to underlying type.
   */
  operator T() const noexcept
  {
    return value_;
  }

  /*!
   * @brief Assignment from underlying type.
   */
  scoped_zero &operator=(T val) noexcept
  {
    value_ = val;
    return *this;
  }

  /*!
   * @brief Securely clear the value.
   */
  void clear() noexcept
  {
    volatile T *vptr = &value_;
    *vptr            = T{};
  }

  // Move semantics
  scoped_zero(scoped_zero &&other) noexcept : value_(other.value_)
  {
    other.clear();
  }

  scoped_zero &operator=(scoped_zero &&other) noexcept
  {
    if (this != &other)
    {
      clear();
      value_ = other.value_;
      other.clear();
    }
    return *this;
  }

  // Copy semantics (allowed for convenience, but clears source)
  scoped_zero(const scoped_zero &other) noexcept : value_(other.value_)
  {
  }

  scoped_zero &operator=(const scoped_zero &other) noexcept
  {
    if (this != &other)
    {
      clear();
      value_ = other.value_;
    }
    return *this;
  }

  // Comparison operators
  bool operator==(const scoped_zero &other) const noexcept
  {
    return value_ == other.value_;
  }

  bool operator!=(const scoped_zero &other) const noexcept
  {
    return value_ != other.value_;
  }

  bool operator==(T other) const noexcept
  {
    return value_ == other;
  }

  bool operator!=(T other) const noexcept
  {
    return value_ != other;
  }
};

} // namespace carbio
