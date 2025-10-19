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

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define CARBIO_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define CARBIO_PAUSE() asm volatile("yield" ::: "memory")
#else
#include <thread>
#define CARBIO_PAUSE() std::this_thread::yield()
#endif

namespace carbio::utility
{
// Helper for static_assert with dependent types
template <bool B, typename T>
inline constexpr bool dependent_bool = B;

/**
 * @brief Control block for lock-free triple buffer
 *
 * Uses bit-packed atomic to track 3 buffer indices:
 * - write_idx: Buffer writer is currently filling
 * - buffer_idx: Middle buffer (swap space)
 * - read_idx: Buffer reader is currently reading
 * - available: True if new data ready for reader
 *
 * IMPORTANT: Must remain small (1 byte) for lock-free atomics on ARM.
 * Cache alignment is applied to the atomic wrapper, not this struct.
 */
struct buffer_control_block
{
  std::uint8_t write_idx : 2;   // 0-2: Which buffer writer uses
  std::uint8_t buffer_idx : 2;  // 0-2: Middle buffer
  std::uint8_t read_idx : 2;    // 0-2: Which buffer reader uses
  bool         available : 2;   // New data available flag

  /**
   * @brief Compute next control block state after write
   * @param block Current control block
   * @return New control block with rotated indices and available=true
   */
  [[nodiscard]] friend buffer_control_block
  write_value(const buffer_control_block &block) noexcept
  {
    return {block.buffer_idx, block.write_idx, block.read_idx, true};
  }

  /**
   * @brief Compute next control block state after read
   * @param block Current control block
   * @return New control block with rotated indices and available=false
   */
  [[nodiscard]] friend buffer_control_block
  read_value(const buffer_control_block &block) noexcept
  {
    return {block.write_idx, block.read_idx, block.buffer_idx, false};
  }
};

/**
 * @brief Lock-free triple buffer
 *
 * Uses atomic compare-exchange on control block for wait-free writes
 * and wait-free reads. Writer never blocks, reader spins until data
 * available. Ideal for real-time sensor polling where latest value
 * matters more than queue depth.
 *
 * Triple buffering ensures:
 * - Writer always has free buffer
 * - Reader always has stable buffer
 * - Middle buffer acts as swap space
 *
 * WARNING: May drop intermediate values - suitable only for polling
 * scenarios where latest value matters (sensor readings, real-time
 * authentication results).
 *
 * @tparam T Value type (must be movable)
 */
template <typename T>
class lockfree_triple_buffer
{
  using control_block_t = std::atomic<buffer_control_block>;
  static_assert(dependent_bool<control_block_t::is_always_lock_free, T>,
                "This queue should be lock-free.");

public:
  /**
   * @brief Push value to writer buffer (never blocks)
   * @param val Value to push (moved into buffer)
   */
  void
  push(T val)
  {
    auto old = _control.load();

    _buffer[old.write_idx] = std::move(val);

    while (!_control.compare_exchange_weak(old, write_value(old)))
      ;
  }

  /**
   * @brief Pop value from reader buffer (spins until available)
   * @return Latest value
   * @note Optimized with CPU-specific pause to reduce bus traffic
   */
  T
  pop()
  {
    // Spinwait with CPU pause to reduce contention
    while (!_control.load(std::memory_order_acquire).available) {
      CARBIO_PAUSE();
    }

    auto old  = _control.load(std::memory_order_acquire);
    auto next = read_value(old);

    while (!_control.compare_exchange_weak(old, next,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
      next = read_value(old);
    }

    return std::move(_buffer[next.read_idx]);
  }

  /**
   * @brief Pop value unless predicate becomes true
   * @tparam Pred Predicate type (e.g., bool())
   * @param p Predicate to check (e.g., cancellation flag)
   * @return Value if available, nullopt if predicate triggered
   */
  template <typename Pred>
  std::optional<T>
  pop_unless(Pred &&p)
  {
    while (!_control.load(std::memory_order_acquire).available) {
      if (p()) {
        break;
      }
      CARBIO_PAUSE();
    }

    auto old = _control.load(std::memory_order_acquire);

    if (!old.available) {
      return std::nullopt;
    }

    auto next = read_value(old);
    while (!_control.compare_exchange_weak(old, next,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
      next = read_value(old);
    }

    return std::move(_buffer[next.read_idx]);
  }

  /**
   * @brief Check if buffer is empty (no data available)
   * @return true if empty, false otherwise
   */
  [[nodiscard]] bool
  empty() const noexcept
  {
    return !_control.load(std::memory_order_acquire).available;
  }

  /**
   * @brief No-op wake (lock-free, no threads to wake)
   */
  void
  wake()
  {
  }

private:
  alignas(64) std::array<T, 3> _buffer;
  alignas(64) control_block_t  _control{{0, 1, 2, false}};  // Initial: write=0, middle=1, read=2, empty
};

#undef CARBIO_PAUSE

} /* namespace carbio::utility */
