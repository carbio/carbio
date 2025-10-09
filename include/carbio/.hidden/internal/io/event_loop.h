#ifndef CARBIO_IO_EVENT_LOOP_H
#define CARBIO_IO_EVENT_LOOP_H

#include <chrono>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <system_error>
#include <unordered_map>

namespace carbio::internal
{

enum class event_type : uint32_t
{
  read  = 0x001,
  write = 0x004,
  error = 0x008
};

class event_loop
{
public:
  using callback = std::function<void()>;

  event_loop();
  ~event_loop();

  event_loop(event_loop const &)            = delete;
  event_loop &operator=(event_loop const &) = delete;
  event_loop(event_loop &&)                 = delete;
  event_loop &operator=(event_loop &&)      = delete;

  // Register file descriptor for events
  [[nodiscard]] std::expected<void, std::error_code> register_fd(int fd, event_type events, callback cb);

  // Unregister file descriptor
  void unregister_fd(int fd);

  // Modify registered fd events
  [[nodiscard]] std::expected<void, std::error_code> modify_fd(int fd, event_type events);

  // Create timer that fires after duration
  [[nodiscard]] std::expected<int, std::error_code> create_timer(std::chrono::milliseconds duration, callback cb);

  // Cancel timer
  void cancel_timer(int timer_fd);

  // Run event loop once (returns true if events were processed)
  [[nodiscard]] bool run_once(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

  // Run event loop until stopped
  void run();

  // Stop the event loop
  void stop();

  // Check if loop is running
  [[nodiscard]] bool is_running() const noexcept
  {
    return m_running;
  }

private:
  struct event_data
  {
    callback cb;
    bool     is_timer{false};
  };

  int                                     m_epoll_fd{-1};
  bool                                    m_running{false};
  std::unordered_map<int, event_data>      m_callbacks;
};

// Awaitable for async sleep
class sleep_awaitable
{
public:
  explicit sleep_awaitable(event_loop &loop, std::chrono::milliseconds duration) : m_loop(loop), m_duration(duration) {}

  bool await_ready() const noexcept
  {
    return m_duration.count() <= 0;
  }

  void await_suspend(std::coroutine_handle<> handle)
  {
    auto result = m_loop.create_timer(m_duration, [handle]() mutable { handle.resume(); });
    if (result)
    {
      m_timer_fd = *result;
    }
  }

  void await_resume() noexcept
  {
    if (m_timer_fd >= 0)
    {
      m_loop.cancel_timer(m_timer_fd);
      m_timer_fd = -1;
    }
  }

private:
  event_loop                  &m_loop;
  std::chrono::milliseconds    m_duration;
  int                          m_timer_fd{-1};
};

} // namespace carbio::internal

#endif // CARBIO_IO_EVENT_LOOP_H
