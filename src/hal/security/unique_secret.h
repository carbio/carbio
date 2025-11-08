#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>

#ifdef __linux__
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace carbio
{
template <typename T>
concept is_trivially_secret = std::is_trivially_copyable_v<T>;

template <typename T>
concept is_contiguous_container = requires(T a)
{
  {
    a.data()
    } -> std::convertible_to<typename T::value_type*>;
  {
    a.size()
    } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept is_string_like = requires(T a)
{
  {
    a.data()
    } -> std::convertible_to<const typename T::value_type*>;
  {
    a.size()
    } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept is_secret_eligible = is_trivially_secret<T> || is_contiguous_container<T> || is_string_like<T>;

template <is_secret_eligible T>
class unique_secret
{
  T v_{};
  bool owns_{false};

public:
  // non-copyable
  unique_secret(const unique_secret& x) = delete;
  unique_secret& operator=(const unique_secret& x) = delete;
  // move-only - exclusive ownership
  unique_secret(unique_secret&& x) noexcept;
  unique_secret& operator=(unique_secret&& x) noexcept;
  // default construct
  unique_secret() noexcept = default;
  // construct from rvalue (preferred)
  explicit unique_secret(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>)
      : v_(std::move(v))
      , owns_(true)
  {
    try_mlock_if_contiguous(v_);
  }

  template <typename... Args>
  requires std::constructible_from<T, Args...>
  void emplace(Args&&... args) {}

private:
  void destroy() noexcept
  {
    if (!owns_)
      return;

    if constexpr (is_trivially_secret<T>)
    {
      zero_memory_impl(static_cast<void*>(std::addressof(v_)), sizeof(T));
    }
    else if constexpr (is_contiguous_container<T>)
    {
      
    }
    else if constexpr (is_string_like<T>)
    {
    }
    else
    {
    }
  }

  template <typename C>
  inline static void try_mlock_if_contiguous(C& c) noexcept
  {
    if constexpr (is_contiguous_container<C> || is_string_like<C>)
    {
      auto sz = c.size();
      if (sz == 0)
        return;
      void* p = const_cast<void*>(static_cast<const void*>(c.data()));
      std::size_t bytes = sz * sizeof(decltype(c[0]));
      long page = sysconf(_SC_PAGESIZE);
      std::uintptr_t s = reinterpret_cast<std::uintptr_t>(p) & ~(page - 1);
      std::size_t n = bytes + (reinterpret_cast<std::uintptr_t>(p) - s);
      (void)mlock(reinterpret_cast<void*>(s), n);
    }
  }
};

template <typename T, typename... Args>
unique_secret<T> make_unique_secret(Args&&... args)
{
  return unique_secret<T>(T(std::forward<Args>(args)...));
}

} // namespace carbio