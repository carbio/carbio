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

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace carbio::utility
{
/**
 * @brief Thread-safe blocking queue for enrollment operations
 *
 * Uses mutex + condition variable for efficient blocking on empty queue.
 * Suitable for multi-stage enrollment where stages may take variable time
 * and message ordering must be preserved.
 *
 * @tparam T Value type (must be movable)
 */
template <typename T>
class blocking_queue
{
public:
  /**
   * @brief Push value to queue and notify one waiting thread
   * @param val Value to push (moved into queue)
   */
  void
  push(T val)
  {
    {
      std::unique_lock lock{m_mutex};
      m_queue.push(std::move(val));
    }
    m_condition.notify_one();
  }

  /**
   * @brief Pop value from queue (blocks if empty)
   * @return Value from front of queue
   */
  T
  pop()
  {
    std::unique_lock lock{m_mutex};
    m_condition.wait(lock, [&] { return !m_queue.empty(); });
    auto r = std::move(m_queue.front());
    m_queue.pop();
    return r;
  }

  /**
   * @brief Pop value unless predicate becomes true
   * @tparam Pred Predicate type (e.g., bool())
   * @param p Predicate to check (e.g., cancellation flag)
   * @return Value if queue not empty, nullopt if predicate triggered
   */
  template <typename Pred>
  std::optional<T>
  pop_unless(Pred &&p)
  {
    std::unique_lock lock{m_mutex};
    m_condition.wait(lock, [&] { return p() || !m_queue.empty(); });

    if (m_queue.empty())
      return std::nullopt;

    auto r = std::move(m_queue.front());
    m_queue.pop();
    return {r};
  }

  /**
   * @brief Check if queue is empty
   * @return true if empty, false otherwise
   */
  [[nodiscard]] bool
  empty() const noexcept
  {
    return m_queue.empty();
  }

  /**
   * @brief Wake all waiting threads (for shutdown)
   */
  void
  wake()
  {
    { std::unique_lock lock{m_mutex}; }
    m_condition.notify_all();
  }

private:
  std::queue<T>           m_queue;
  std::mutex              m_mutex;
  std::condition_variable m_condition;
};

} /* namespace carbio::utility */
