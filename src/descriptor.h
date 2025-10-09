#ifndef CARBIO_DESCRIPTOR_H
#define CARBIO_DESCRIPTOR_H

#include "descriptor_traits.h"

#include <utility>

namespace carbio
{
template <typename Traits>
class descriptor
{
public:
  using traits_type = Traits;
  using native_type = typename Traits::native_type;

  descriptor() noexcept : handle_(traits_type::invalid())
  {
  }

  explicit descriptor(native_type new_handle) noexcept : handle_(new_handle)
  {
  }

  descriptor(descriptor &&other) noexcept : handle_(std::exchange(other.handle_, traits_type::invalid()))
  {
  }

  descriptor &
  operator=(descriptor &&other) noexcept
  {
    reset(std::exchange(other.handle_, traits_type::invalid()));
    return *this;
  }

  ~descriptor() noexcept
  {
    reset();
  }

  descriptor(const descriptor &)            = delete;
  descriptor &operator=(const descriptor &) = delete;

  [[nodiscard]] inline constexpr bool
  is_valid() const noexcept
  {
    return traits_type::valid(handle_);
  }

  [[nodiscard]] inline constexpr const native_type &
  native_handle() const noexcept
  {
    return handle_;
  }

  inline constexpr void
  reset(native_type new_handle = traits_type::invalid()) noexcept
  {
    auto obsolete_handle = std::exchange(handle_, new_handle);
    if (traits_type::valid(obsolete_handle))
      traits_type::close(obsolete_handle);
  }

  inline constexpr native_type
  release() noexcept
  {
    return std::exchange(handle_, traits_type::invalid());
  }

private:
  native_type handle_{traits_type::invalid};
};

using file_descriptor = descriptor<file_descriptor_traits>;
} /* namespace carbio */

#endif // CARBIO_DESCRIPTOR_H
