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

#include <cmath>
#include <concepts>
#include <limits>

namespace carbio::utility {
/*!
 * @brief Determines whether two floating point values are similar but within
 * absolute threshold.
 *
 * With other words, \c approximately_equal gives whether the difference between
 * \c x and \c y is smaller than the acceptable error \c tolerance </c>
 * determined by the larger of \c x or \c y. This means that the two values are
 * "close enough", and we can say that they're approximately equal.
 *
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar
 * (|x-y|<tolerance).
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against
 * different magnitudes.
 * @warning Use approximately_zero if you want to compare against a zero.
 */
template <std::floating_point T>
inline constexpr bool
approximately_equal(T x, T y,
                    T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
  return std::abs(x - y) <=
         ((std::abs(x) < std::abs(y) ? std::abs(y) : std::abs(x)) * tolerance);
}

/*!
 * @brief Checks if a floating point number is approximately zero.
 * @tparam \c T an arbitrary floating point type.
 * @param x The floating point number.
 * @param tolerance Minimum threshold to state that \c x is close to zero
 * (|x|<tolerance)
 * @returns \c true if <c> x < tolerance </c>, \c false otherwise.
 */
template <std::floating_point T>
inline constexpr bool
approximately_zero(T x,
                   T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
  return std::abs(x) <= tolerance;
}

/*!
 * @brief Checks if two floating point types are essentially equal.
 *
 * With other words, \c essentially_equal </c> gives whether the difference
 * between \c x and \c y is smaller than the acceptable error \c tolerance
 * determined by the smaller of \c x or \c y. This means that the values differ
 * less than the acceptable difference in any calculation, so that perhaps
 * they're not actually equal, but they are "essentially equal".
 *
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar
 * (|x-y|<tolerance*min(x,y))
 * @returns \c true if <c> abs(x-y)<tolerance*min(x,y) </c>, \c false otherwise.
 * @warning Better use approximately_zero() if you want to compare against zero.
 */
template <std::floating_point T>
inline constexpr bool
essentially_equal(T x, T y,
                  T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
  return std::abs(x - y) <=
         ((std::abs(x) > std::abs(y) ? std::abs(y) : std::abs(x)) * tolerance);
}

/*!
 * @brief Checks if \c x floating point number is definitely greater than \c y
 * floating point number.
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar
 * (|x- y| > tolerance * max(x,y))
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against
 * different magnitudes
 */
template <std::floating_point T>
inline constexpr bool definitely_greater_than(
    T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
  return (x - y) <=
         ((std::abs(x) < std::abs(y) ? std::abs(y) : std::abs(x)) * tolerance);
}

/*!
 * @brief Checks if \c x floating point number is definitely less than \c y
 * floating point number.
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar
 * (|x-y|<tolerance)
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against
 * different magnitudes
 */
template <std::floating_point T>
inline constexpr bool
definitely_less_than(T x, T y,
                     T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
  return (y - x) <=
         ((std::abs(x) < std::abs(y) ? std::abs(y) : std::abs(x)) * tolerance);
}
} // namespace carbio::utility
