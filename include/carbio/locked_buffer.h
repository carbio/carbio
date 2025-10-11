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

#ifndef CARBIO_LOCKED_BUFFER_H
#define CARBIO_LOCKED_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace carbio
{
/*!
 * @brief Secure memory buffer with automatic locking and clearing.
 *
 * This class provides a secure container for sensitive data (encryption keys,
 * passwords, etc.) with the following security features:
 * - Memory locking (mlock) to prevent swapping to disk
 * - Automatic secure clearing on destruction
 * - No copy semantics (move-only)
 * - Volatile memory access to prevent compiler optimizations
 *
 * @tparam T Element type (typically std::uint8_t)
 */
template <typename T = std::uint8_t>
class locked_buffer
{
  struct secure_deleter
  {
    std::size_t size_;
    bool        was_locked_;

    void operator()(T *ptr) const noexcept
    {
      if (ptr)
      {
        // securely clear memory before deletion
        volatile T *vptr = ptr;
        for (std::size_t i = 0; i < size_; ++i)
        {
          vptr[i] = T{};
        }

#ifdef __linux__
        if (was_locked_)
        {
          munlock(ptr, size_ * sizeof(T));
        }
#endif

        delete[] ptr;
      }
    }
  };

  std::unique_ptr<T[], secure_deleter> data_;
  std::size_t                           size_;

public:
  /*!
   * @brief Construct an empty locked buffer.
   */
  locked_buffer() noexcept : data_(nullptr), size_(0)
  {
  }

  /*!
   * @brief Construct a locked buffer with specified size.
   * @param size Number of elements to allocate.
   */
  explicit locked_buffer(std::size_t size) noexcept : data_(nullptr), size_(0)
  {
    if (size > 0)
    {
      T *raw_ptr = new (std::nothrow) T[size]();
      if (raw_ptr)
      {
        size_ = size;

#ifdef __linux__
        const bool locked = (mlock(raw_ptr, size_ * sizeof(T)) == 0);
        data_ = std::unique_ptr<T[], secure_deleter>(raw_ptr, secure_deleter{size_, locked});
#else
        data_ = std::unique_ptr<T[], secure_deleter>(raw_ptr, secure_deleter{size_, false});
#endif
      }
    }
  }

  /*!
   * @brief Destructor - securely clears and unlocks memory automatically via unique_ptr.
   */
  ~locked_buffer() noexcept = default;

  // move semantics
  locked_buffer(locked_buffer &&other) noexcept : data_(std::move(other.data_)), size_(other.size_)
  {
    other.size_ = 0;
  }

  locked_buffer &operator=(locked_buffer &&other) noexcept
  {
    if (this != &other)
    {
      data_       = std::move(other.data_);
      size_       = other.size_;
      other.size_ = 0;
    }
    return *this;
  }

  // no copy semantics
  locked_buffer(const locked_buffer &)            = delete;
  locked_buffer &operator=(const locked_buffer &) = delete;

  // --- accessors

  [[nodiscard]] T *data() noexcept
  {
    return data_.get();
  }

  [[nodiscard]] const T *data() const noexcept
  {
    return data_.get();
  }

  [[nodiscard]] std::size_t size() const noexcept
  {
    return size_;
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return size_ == 0 || data_.get() == nullptr;
  }

  [[nodiscard]] bool is_locked() const noexcept
  {
    return data_ && data_.get_deleter().was_locked_;
  }

  // --- operations

  [[nodiscard]] bool resize(std::size_t new_size) noexcept
  {
    // destroy existing data
    data_.reset();
    size_ = 0;

    // allocate new memory
    if (new_size > 0)
    {
      T *raw_ptr = new (std::nothrow) T[new_size]();
      if (raw_ptr)
      {
        size_ = new_size;

#ifdef __linux__
        const bool locked = (mlock(raw_ptr, new_size * sizeof(T)) == 0);
        data_ = std::unique_ptr<T[], secure_deleter>(raw_ptr, secure_deleter{new_size, locked});
#else
        data_ = std::unique_ptr<T[], secure_deleter>(raw_ptr, secure_deleter{new_size, false});
#endif

        return true;
      }
      return false;
    }

    return true;
  }

  void clear() noexcept
  {
    if (data_ && size_ > 0)
    {
      volatile T *vptr = data_.get();
      for (std::size_t i = 0; i < size_; ++i)
      {
        vptr[i] = T{};
      }
    }
  }

  void copy_from(const T *src, std::size_t count) noexcept
  {
    if (data_ && src && count <= size_)
    {
      std::memcpy(data_.get(), src, count * sizeof(T));
    }
  }

  void fill(T value) noexcept
  {
    if (data_ && size_ > 0)
    {
      std::fill_n(data_.get(), size_, value);
    }
  }

  // --- iterators

  [[nodiscard]] T *begin() noexcept
  {
    return data_.get();
  }
  [[nodiscard]] const T *begin() const noexcept
  {
    return data_.get();
  }
  [[nodiscard]] T *end() noexcept
  {
    return data_.get() ? data_.get() + size_ : nullptr;
  }
  [[nodiscard]] const T *end() const noexcept
  {
    return data_.get() ? data_.get() + size_ : nullptr;
  }

  // --- operators

  [[nodiscard]] T &operator[](std::size_t index) noexcept
  {
    return data_[index];
  }
  [[nodiscard]] const T &operator[](std::size_t index) const noexcept
  {
    return data_[index];
  }
};

} // namespace carbio

#endif // CARBIO_LOCKED_BUFFER_H
