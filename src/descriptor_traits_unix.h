#ifndef CARBIO_DESCRIPTOR_TRAITS_UNIX_H
#define CARBIO_DESCRIPTOR_TRAITS_UNIX_H

#include <unistd.h>

namespace carbio
{
struct file_descriptor_traits
{
  using native_type = int;

  inline static constexpr native_type
  invalid() noexcept
  {
    return -1;
  }

  inline static constexpr bool
  valid(native_type handle) noexcept
  {
    return invalid() != handle;
  }

  inline static void
  close(native_type handle) noexcept
  {
    if (valid(handle))
      ::close(handle);
  }
};
} // namespace carbio

#endif // CARBIO_DESCRIPTOR_TRAITS_UNIX_H
