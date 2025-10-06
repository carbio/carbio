#ifndef CARBIO_FLOATING_POINT_H
#define CARBIO_FLOATING_POINT_H

#include <cfloat>
#include <cmath>
#include <limits>

namespace carbio
{
inline constexpr bool nearlyEqual(float x, float y, float tolerance = std::numeric_limits<float>::epsilon()) noexcept
{
  return std::abs(x - y) < tolerance;
}

inline constexpr bool nearlyEqual(double x, double y, double tolerance = std::numeric_limits<double>::epsilon()) noexcept
{
  return std::abs(x - y) < tolerance;
}
} // namespace carbio

#endif // CARBIO_FLOATING_POINT_H
