#include "carbio/internal/io/event_loop.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace carbio::internal
{

event_loop::event_loop()
{
  m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (m_epoll_fd < 0)
  {
    // Constructor can't return error, but we check in operations
  }
}

event_loop::~event_loop()
{
  stop();

  // Close all registered timers
  for (auto const &[fd, data] : m_callbacks)
  {
    if (data.is_timer)
    {
      ::close(fd);
    }
  }

  if (m_epoll_fd >= 0)
  {
    ::close(m_epoll_fd);
  }
}

std::expected<void, std::error_code> event_loop::register_fd(int fd, event_type events, callback cb)
{
  if (m_epoll_fd < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  struct epoll_event ev = {};
  ev.events             = EPOLLET; // Edge-triggered for efficiency

  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::read)) != 0)
  {
    ev.events |= EPOLLIN;
  }
  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::write)) != 0)
  {
    ev.events |= EPOLLOUT;
  }
  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::error)) != 0)
  {
    ev.events |= EPOLLERR | EPOLLHUP;
  }

  ev.data.fd = fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  m_callbacks[fd] = event_data{.cb = std::move(cb), .is_timer = false};
  return {};
}

void event_loop::unregister_fd(int fd)
{
  if (m_epoll_fd >= 0)
  {
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
  }
  m_callbacks.erase(fd);
}

std::expected<void, std::error_code> event_loop::modify_fd(int fd, event_type events)
{
  if (m_epoll_fd < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  struct epoll_event ev = {};
  ev.events             = EPOLLET;

  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::read)) != 0)
  {
    ev.events |= EPOLLIN;
  }
  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::write)) != 0)
  {
    ev.events |= EPOLLOUT;
  }
  if ((static_cast<uint32_t>(events) & static_cast<uint32_t>(event_type::error)) != 0)
  {
    ev.events |= EPOLLERR | EPOLLHUP;
  }

  ev.data.fd = fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  return {};
}

std::expected<int, std::error_code> event_loop::create_timer(std::chrono::milliseconds duration, callback cb)
{
  int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timer_fd < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  struct itimerspec spec = {};
  auto seconds           = std::chrono::duration_cast<std::chrono::seconds>(duration);
  auto nanoseconds       = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

  spec.it_value.tv_sec  = seconds.count();
  spec.it_value.tv_nsec = nanoseconds.count();

  if (timerfd_settime(timer_fd, 0, &spec, nullptr) < 0)
  {
    ::close(timer_fd);
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  struct epoll_event ev = {};
  ev.events             = EPOLLIN | EPOLLET;
  ev.data.fd            = timer_fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) < 0)
  {
    ::close(timer_fd);
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  m_callbacks[timer_fd] = event_data{.cb = std::move(cb), .is_timer = true};
  return timer_fd;
}

void event_loop::cancel_timer(int timer_fd)
{
  if (timer_fd >= 0)
  {
    unregister_fd(timer_fd);
    ::close(timer_fd);
  }
}

bool event_loop::run_once(std::chrono::milliseconds timeout)
{
  if (m_epoll_fd < 0)
  {
    return false;
  }

  constexpr int MAX_EVENTS = 64;
  struct epoll_event events[MAX_EVENTS];

  int timeout_ms = timeout.count() < 0 ? -1 : static_cast<int>(timeout.count());
  int nfds      = epoll_wait(m_epoll_fd, events, MAX_EVENTS, timeout_ms);

  if (nfds < 0)
  {
    if (errno == EINTR)
    {
      return false; // Interrupted, try again
    }
    return false;
  }

  bool processed_events = false;

  for (int i = 0; i < nfds; ++i)
  {
    int fd = events[i].data.fd;

    auto it = m_callbacks.find(fd);
    if (it != m_callbacks.end())
    {
      auto cb = it->second.cb; // Copy callback in case it modifies m_callbacks
      bool is_timer  = it->second.is_timer;

      // Timer cleanup
      if (is_timer)
      {
        uint64_t expirations;
        ::read(fd, &expirations, sizeof(expirations)); // Clear timer event
        cancel_timer(fd);
      }

      if (cb)
      {
        cb();
        processed_events = true;
      }
    }
  }

  return processed_events;
}

void event_loop::run()
{
  m_running = true;
  while (m_running)
  {
    run_once(std::chrono::milliseconds{100});
  }
}

void event_loop::stop()
{
  m_running = false;
}

} // namespace carbio::internal
