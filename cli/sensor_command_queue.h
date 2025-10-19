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
#include "carbio/utility/spinlock.h"

#include "sensor_command.h"

#include <atomic>
#include <chrono>
#include <queue>
#include <thread>
#include <vector>

class SensorCommandQueue {
public:
  SensorCommandQueue() = default;
  ~SensorCommandQueue() = default;

  SensorCommandQueue(const SensorCommandQueue &) = delete;
  SensorCommandQueue &operator=(const SensorCommandQueue &) = delete;
  SensorCommandQueue(SensorCommandQueue &&) = delete;
  SensorCommandQueue &operator=(SensorCommandQueue &&) = delete;

  void push(SensorCommand cmd) {
    std::unique_lock lock(m_spinlock);
    m_queue.push(std::move(cmd));
    m_hasData.store(true, std::memory_order_release);
  }

  SensorCommand pop() {
    while (!m_hasData.load(std::memory_order_acquire)) {
      cpu_pause();
    }

    std::unique_lock lock(m_spinlock);
    SensorCommand cmd = std::move(const_cast<SensorCommand &>(m_queue.top()));
    m_queue.pop();

    if (m_queue.empty()) {
      m_hasData.store(false, std::memory_order_release);
    }

    return cmd;
  }

  bool tryPop(SensorCommand &out) {
    if (!m_hasData.load(std::memory_order_acquire)) {
      return false;
    }

    std::unique_lock lock(m_spinlock);
    if (m_queue.empty()) {
      m_hasData.store(false, std::memory_order_release);
      return false;
    }

    out = std::move(const_cast<SensorCommand &>(m_queue.top()));
    m_queue.pop();

    if (m_queue.empty()) {
      m_hasData.store(false, std::memory_order_release);
    }

    return true;
  }

  void clearType(CommandType type) {
    std::unique_lock lock(m_spinlock);

    std::vector<SensorCommand> temp;
    while (!m_queue.empty()) {
      SensorCommand cmd = std::move(const_cast<SensorCommand &>(m_queue.top()));
      m_queue.pop();
      if (cmd.type != type) {
        temp.push_back(std::move(cmd));
      }
    }

    for (auto &cmd : temp) {
      m_queue.push(std::move(cmd));
    }

    if (m_queue.empty()) {
      m_hasData.store(false, std::memory_order_release);
    }
  }

  void clearLowPriority() {
    std::unique_lock lock(m_spinlock);

    std::vector<SensorCommand> temp;
    while (!m_queue.empty()) {
      SensorCommand cmd = std::move(const_cast<SensorCommand &>(m_queue.top()));
      m_queue.pop();
      if (cmd.priority != CommandPriority::Low) {
        temp.push_back(std::move(cmd));
      }
    }

    for (auto &cmd : temp) {
      m_queue.push(std::move(cmd));
    }

    if (m_queue.empty()) {
      m_hasData.store(false, std::memory_order_release);
    }
  }

  bool empty() const { return !m_hasData.load(std::memory_order_acquire); }

  size_t size() const {
    std::unique_lock lock(m_spinlock);
    return m_queue.size();
  }

private:
  mutable carbio::utility::spinlock m_spinlock;
  alignas(64) std::atomic<bool> m_hasData{false};
  std::priority_queue<SensorCommand, std::vector<SensorCommand>,
                      SensorCommandComparator>
      m_queue;
};
