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

#include "carbio/utility/cpu_hints.h"

#include <atomic>
#include <cstdint>

namespace carbio::utility {

/// @brief Cache-aligned hardware spinlock for ultra-low latency critical sections
/// @details Optimized for short-duration locks (<1μs). Uses CPU-specific pause
///          instructions to reduce memory bus traffic and power consumption.
///          AUTOMOTIVE-GRADE: Suitable for real-time control paths.
class alignas(64) spinlock final {
public:
    spinlock() noexcept = default;
    ~spinlock() noexcept = default;

    // Non-copyable, non-movable
    spinlock(const spinlock &) = delete;
    spinlock &operator=(const spinlock &) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    /// @brief Acquire lock with exponential backoff
    /// @details Uses aggressive spinning for first iterations, then yields
    ///          Marked noinline to prevent complex nested loop optimizations
    ///          that trigger strict-overflow warnings with automotive-grade flags
    __attribute__((noinline)) void lock() noexcept {
        constexpr std::uint32_t MAX_SPINS = 32;
        std::uint32_t spin_count = 0;

        while (true) {
            // Try to acquire
            if (!locked_.exchange(true, std::memory_order_acquire)) {
                return;
            }

            // Exponential backoff with pause
            if (spin_count < MAX_SPINS) {
                for (std::uint32_t i = 0; i < (1u << spin_count); ++i) {
                    cpu_pause();
                }
                ++spin_count;
            } else {
                // After max spins, yield to scheduler
                cpu_pause();
            }
        }
    }

    /// @brief Try to acquire lock without blocking
    /// @return true if lock acquired, false otherwise
    [[nodiscard]] inline bool try_lock() noexcept {
        return !locked_.exchange(true, std::memory_order_acquire);
    }

    /// @brief Release lock
    inline void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }

private:
    alignas(64) std::atomic<bool> locked_{false};
};

} // namespace carbio::utility

#undef CARBIO_SPINLOCK_PAUSE
