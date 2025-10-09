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

#ifndef CARBIO_FINGERPRINT_RESULT_H
#define CARBIO_FINGERPRINT_RESULT_H

#include "carbio/internal/fingerprint/status_code.h"

#include <expected>

namespace carbio::internal
{
/*!
 * @brief Represents the possible outcome of an operation that may succeed or fail.
 * This type holds either:
 * - a computational value of the operation of type `T`
 * - a status code of the operation
 * @tparam T The type of the value of operation outcome.
 * @note Use void_result for such result that does not produce a meaningful value.
 */
template <typename T>
using result = std::expected<T, status_code>;

/*!
 * @brief Represents the possible outcome of an operation that may succeed or fail.
 * This type is a specialization of result type that does not produce meaningful
 * value on successful operation.
 * This allows consistent use of monadic interface even for chainable operations
 * that depends on side-effects.
 */
using void_result = result<void>;

/*!
 * @brief Constructs a result that represents a successful outcome.
 *
 * This function automatically deduces the type of value and wraps it in `result`.
 *
 * @tparam The type of the value of operation outcome.
 * @param value The value in a successful result.
 * @returns A successful result that wraps the value.
 * @note This function never throws.
 */
template <typename T>
[[nodiscard]] inline constexpr auto make_success(T&& value) noexcept
{
  using value_type = std::remove_cvref_t<T>;
  return result<value_type>(std::forward<T>(value));
}

/*!
 * @brief Constructs a void result that represents a successful outcome.
 * Use this function for operations that succeed but does not produce a value.
 * @returns A successful void result.
 * @note This function never throws.
 */
[[nodiscard]] inline constexpr auto make_success(void) noexcept
{
  return void_result();
}

/*!
 * @brief Construct a result that represents an error condition.
 * @param status The status code representing the error condition.
 * @returns A result that represents error condition.
 * @note This function never throws.
 */
template <typename T>
[[nodiscard]] inline constexpr auto make_error(status_code status) noexcept
{
  return result<T>(std::unexpected(status));
}

/*!
 * @brief Construct a void result that represents an error condition.
 * @param status The status code representing the error condition.
 * @returns A void result that represents error condition.
 * @note This function never throws.
 */
[[nodiscard]] inline constexpr auto make_error(status_code status) noexcept
{
  return void_result(std::unexpected(status));
}
} /* namespace carbio::internal */

#endif /* CARBIO_FINGERPRINT_RESULT_H */