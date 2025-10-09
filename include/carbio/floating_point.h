#ifndef CARBIO_FLOATING_POINT_H
#define CARBIO_FLOATING_POINT_H

#include <concepts>
#include <cmath>
#include <limits>

namespace carbio
{
/*!
 * @brief Determines whether two floating point values are similar but within absolute threshold.
 *
 * With other words, \c approximately_equal gives whether the difference between \c x and \c y
 * is smaller than the acceptable error \c tolerance </c> determined by the larger of \c x or \c y.
 * This means that the two values are "close enough", and we can say that they're approximately equal. 
 *
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar (|x-y|<tolerance).
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against different magnitudes.
 * @warning Use approximately_zero if you want to compare against a zero.
 */
template<std::floating_point T>
inline constexpr bool approximately_equal(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) noexcept
{
  return std::abs<T>(x - y) <= ((std::abs<T>(x) < std::abs<T>(y) ? std::abs<T>(y) : std::abs<T>(x)) * tolerance);
}

/*!
 * @brief Checks if a floating point number is approximately zero.
 * @tparam \c T an arbitrary floating point type.
 * @param x The floating point number.
 * @param tolerance Minimum threshold to state that \c x is close to zero (|x|<tolerance)
 * @returns \c true if <c> x < tolerance </c>, \c false otherwise.
 */
template<std::floating_point T>
inline constexpr bool approximately_zero(T x, T tolerance = std::numeric_limits<T>::epsilon()) noexcept
{
  return std::abs<T>(x) <= tolerance;
}

/*!
 * @brief Checks if two floating point types are essentially equal.
 *
 * With other words, \c essentially_equal </c> gives whether the difference between \c x and \c y
 * is smaller than the acceptable error \c tolerance determined by the smaller of \c x or \c y.
 * This means that the values differ less than the acceptable difference in any calculation, so that
 * perhaps they're not actually equal, but they are "essentially equal".
 *
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar (|x-y|<tolerance*min(x,y))
 * @returns \c true if <c> abs(x-y)<tolerance*min(x,y) </c>, \c false otherwise.
 * @warning Better use approximately_zero() if you want to compare against zero.
 */
template<std::floating_point T>
inline constexpr bool essentially_equal(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) noexcept
{
  return std::abs<T>(x - y) <= ((std::abs<T>(x) > std::abs<T>(y) ? std::abs<T>(y) : std::abs<T>(x)) * tolerance);
}

/*!
 * @brief Checks if \c x floating point number is definitely greater than \c y floating point number.
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar (|x- y| > tolerance * max(x,y))
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against different magnitudes
 */
template<std::floating_point T>
inline constexpr bool definitely_greater_than(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) noexcept
{
  return (x - y) <= ((std::abs<T>(x) < std::abs<T>(y) ? std::abs<T>(y) : std::abs<T>(x)) * tolerance);
}

/*!
 * @brief Checks if \c x floating point number is definitely less than \c y floating point number.
 * @tparam \c T an arbitrary floating point type.
 * @param x The first floating point number.
 * @param y The second floating point number.
 * @param tolerance Minimum threshold to state that \c x and \c y are similar (|x-y|<tolerance)
 * @returns \c true if <c> abs(x-y)<tolerance </c>, \c false otherwise.
 * @warning Better use relative comparisons to be much more robust against different magnitudes
 */
template<std::floating_point T>
inline constexpr bool definitely_less_than(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) noexcept
{
  return (y - x) <= ((std::abs<T>(x) < std::abs<T>(y) ? std::abs<T>(y) : std::abs<T>(x)) * tolerance);
}


} // namespace carbio

#endif // CARBIO_FLOATING_POINT_H
