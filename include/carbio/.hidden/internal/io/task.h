#ifndef CARBIO_IO_TASK_H
#define CARBIO_IO_TASK_H

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace carbio::internal
{

template <typename T>
class task
{
public:
  struct promise_type
  {
    std::optional<T>             value{};
    std::exception_ptr           exception{};
    std::coroutine_handle<>      continuation{};

    task get_return_object()
    {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept
    {
      return {};
    }

    auto final_suspend() noexcept
    {
      struct awaiter
      {
        bool await_ready() noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
        {
          if (h.promise().continuation)
          {
            return h.promise().continuation;
          }
          return std::noop_coroutine();
        }

        void await_resume() noexcept {}
      };
      return awaiter{};
    }

    void return_value(T val)
    {
      value = std::move(val);
    }

    void unhandled_exception()
    {
      exception = std::current_exception();
    }
  };

  using handle_type = std::coroutine_handle<promise_type>;

  explicit task(handle_type h) : m_handle(h) {}

  task(task &&other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}

  task &operator=(task &&other) noexcept
  {
    if (this != &other)
    {
      if (m_handle)
      {
        m_handle.destroy();
      }
      m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
  }

  ~task()
  {
    if (m_handle)
    {
      m_handle.destroy();
    }
  }

  task(task const &)            = delete;
  task &operator=(task const &) = delete;

  bool await_ready() const noexcept
  {
    return m_handle.done();
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    m_handle.promise().continuation = continuation;
    return m_handle;
  }

  T await_resume()
  {
    if (m_handle.promise().exception)
    {
      std::rethrow_exception(m_handle.promise().exception);
    }
    return std::move(*m_handle.promise().value);
  }

  void resume()
  {
    if (m_handle && !m_handle.done())
    {
      m_handle.resume();
    }
  }

  bool done() const noexcept
  {
    return m_handle.done();
  }

  T get()
  {
    if (!m_handle.done())
    {
      m_handle.resume();
    }
    return await_resume();
  }

private:
  handle_type m_handle;
};

// Specialization for void
template <>
class task<void>
{
public:
  struct promise_type
  {
    std::exception_ptr      exception{};
    std::coroutine_handle<> continuation{};

    task get_return_object()
    {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept
    {
      return {};
    }

    auto final_suspend() noexcept
    {
      struct awaiter
      {
        bool await_ready() noexcept
        {
          return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
        {
          if (h.promise().continuation)
          {
            return h.promise().continuation;
          }
          return std::noop_coroutine();
        }

        void await_resume() noexcept {}
      };
      return awaiter{};
    }

    void return_void() noexcept {}

    void unhandled_exception()
    {
      exception = std::current_exception();
    }
  };

  using handle_type = std::coroutine_handle<promise_type>;

  explicit task(handle_type h) : m_handle(h) {}

  task(task &&other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}

  task &operator=(task &&other) noexcept
  {
    if (this != &other)
    {
      if (m_handle)
      {
        m_handle.destroy();
      }
      m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
  }

  ~task()
  {
    if (m_handle)
    {
      m_handle.destroy();
    }
  }

  task(task const &)            = delete;
  task &operator=(task const &) = delete;

  bool await_ready() const noexcept
  {
    return m_handle.done();
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    m_handle.promise().continuation = continuation;
    return m_handle;
  }

  void await_resume()
  {
    if (m_handle.promise().exception)
    {
      std::rethrow_exception(m_handle.promise().exception);
    }
  }

  void resume()
  {
    if (m_handle && !m_handle.done())
    {
      m_handle.resume();
    }
  }

  bool done() const noexcept
  {
    return m_handle.done();
  }

  void get()
  {
    if (!m_handle.done())
    {
      m_handle.resume();
    }
    await_resume();
  }

private:
  handle_type m_handle;
};

} // namespace carbio::internal

#endif // CARBIO_IO_TASK_H
