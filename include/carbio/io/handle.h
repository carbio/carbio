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

#include "carbio/io/handle_traits.h"

#include <utility>

namespace carbio::io {
template <typename Traits> class handle_guard {
public:
  using native_handle =
      typename Traits::native_handle; /*!< Alias to native handle type */
  inline static constexpr native_handle invalid_handle =
      Traits::invalid_handle; /*!< Invalid handle value */

  /*!
   * @brief Constructs handle.
   */
  inline constexpr handle_guard() noexcept = default;

  /*!
   * @brief Constructs handle.
   * @param new_handle The new handle that sets handle to.
   */
  explicit handle_guard(native_handle new_handle) noexcept
      : handle_(new_handle) {}

  /*!
   * @brief Destroys handle.
   */
  ~handle_guard() noexcept { reset(); }

  /*!
   * @brief Move-constructs handle.
   * @param other The moved-from handle.
   */
  inline constexpr handle_guard(handle_guard &&other) noexcept
      : handle_(std::exchange(other.handle_, invalid_handle)) {}

  /*!
   * @brief Move-assigns the handle.
   * @param other The moved-from handle.
   * @return The new handle instance.
   */
  inline constexpr handle_guard &operator=(handle_guard &&other) noexcept {
    reset(std::exchange(other.handle_, invalid_handle));
    return *this;
  }

  /*!
   * @brief Convert underlying handle value to logical value.
   * @return true if the underlying handle value is valid, false otherwise.
   */
  inline constexpr operator bool() const noexcept {
    return invalid_handle != handle_;
  }

  /*!
   * @brief Determines whether the underlying handle value is valid or not.
   * @return true if the underlying handle value is valid, false otherwise.
   */
  inline constexpr bool is_valid() const noexcept {
    return invalid_handle != handle_;
  }

  /*!
   * @brief Get the underlying handle value.
   * @return The underlying handle value.
   */
  inline constexpr native_handle get() const noexcept { return handle_; }

  /*!
   * @brief Reset handle.
   * @param new_handle The new handle.
   */
  inline constexpr void reset(native_handle new_handle = invalid_handle) {
    auto old_handle = std::exchange(handle_, new_handle);
    if (invalid_handle != old_handle)
      Traits::close(old_handle);
  }

  /*!
   * @brief Release ownership of the handle.
   * @return The underlying native handle.
   */
  inline constexpr native_handle release() noexcept {
    return std::exchange(handle_, invalid_handle);
  }

  // -- noncopyable
  handle_guard(const handle_guard &other) = delete;
  handle_guard &operator=(const handle_guard &other) = delete;

private:
  native_handle handle_{invalid_handle};
};

using unique_handle = handle_guard<handle_traits>;

} // namespace carbio::io