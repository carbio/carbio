#include "carbio/internal/io/serial_port.h"

#include <fcntl.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace carbio::internal
{

serial_port::serial_port(event_loop &loop) : m_loop(loop) {}

serial_port::~serial_port()
{
  close();
}

std::expected<void, std::error_code> serial_port::open(std::string_view device_path, serial_config const &config)
{
  if (is_open())
  {
    close();
  }

  // Open with non-blocking flag for async I/O
  m_fd = ::open(device_path.data(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
  if (m_fd < 0)
  {
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  // Configure termios
  struct termios tty = {};
  if (tcgetattr(m_fd, &tty) < 0)
  {
    ::close(m_fd);
    m_fd = -1;
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  // Set baud rate
  speed_t baud_rate;
  switch (config.baud_rate)
  {
    case 9600:
      baud_rate = B9600;
      break;
    case 19200:
      baud_rate = B19200;
      break;
    case 38400:
      baud_rate = B38400;
      break;
    case 57600:
      baud_rate = B57600;
      break;
    case 115200:
      baud_rate = B115200;
      break;
    default:
      ::close(m_fd);
      m_fd = -1;
      return std::unexpected(std::error_code{EINVAL, std::system_category()});
  }

  cfsetispeed(&tty, baud_rate);
  cfsetospeed(&tty, baud_rate);

  // Configure data bits
  tty.c_cflag &= ~CSIZE;
  switch (config.data_bits_val)
  {
    case data_bits::five:
      tty.c_cflag |= CS5;
      break;
    case data_bits::six:
      tty.c_cflag |= CS6;
      break;
    case data_bits::seven:
      tty.c_cflag |= CS7;
      break;
    case data_bits::eight:
      tty.c_cflag |= CS8;
      break;
  }

  // Configure parity
  switch (config.parity_val)
  {
    case parity::none:
      tty.c_cflag &= ~PARENB;
      break;
    case parity::even:
      tty.c_cflag |= PARENB;
      tty.c_cflag &= ~PARODD;
      break;
    case parity::odd:
      tty.c_cflag |= PARENB;
      tty.c_cflag |= PARODD;
      break;
  }

  // Configure stop bits
  if (config.stop_bits_val == stop_bits::two)
  {
    tty.c_cflag |= CSTOPB;
  }
  else
  {
    tty.c_cflag &= ~CSTOPB;
  }

  // Hardware flow control
  if (config.hardware_flow_control)
  {
    tty.c_cflag |= CRTSCTS;
  }
  else
  {
    tty.c_cflag &= ~CRTSCTS;
  }

  // Raw mode configuration
  tty.c_cflag |= (CLOCAL | CREAD); // Enable receiver, ignore modem control
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
  tty.c_oflag &= ~OPOST;

  // Non-blocking reads: VMIN=0, VTIME=5 (0.5 seconds for RPi5 RP1 chip)
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 5; // Increased from typical 3 for RPi5 RP1 timing

  // Apply settings
  if (tcsetattr(m_fd, TCSANOW, &tty) < 0)
  {
    ::close(m_fd);
    m_fd = -1;
    return std::unexpected(std::error_code{errno, std::system_category()});
  }

  // Enable low latency mode for RPi5 RP1 chip
  if (config.low_latency)
  {
    struct serial_struct serial;
    if (ioctl(m_fd, TIOCGSERIAL, &serial) == 0)
    {
      serial.flags |= ASYNC_LOW_LATENCY;
      ioctl(m_fd, TIOCSSERIAL, &serial);
    }
  }

  // Flush any existing data
  tcflush(m_fd, TCIOFLUSH);

  return {};
}

void serial_port::close()
{
  if (m_registered)
  {
    m_loop.unregister_fd(m_fd);
    m_registered = false;
  }

  if (m_fd >= 0)
  {
    ::close(m_fd);
    m_fd = -1;
  }
}

std::expected<size_t, std::error_code> serial_port::read_exact(std::span<uint8_t> buffer,
                                                              std::chrono::milliseconds timeout)
{
  if (!is_open())
  {
    return std::unexpected(std::error_code{EBADF, std::system_category()});
  }

  size_t totalRead = 0;
  auto   startTime = std::chrono::steady_clock::now();

  while (totalRead < buffer.size())
  {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);
    if (elapsed >= timeout)
    {
      return std::unexpected(std::error_code{ETIMEDOUT, std::system_category()});
    }

    ssize_t n = ::read(m_fd, buffer.data() + totalRead, buffer.size() - totalRead);

    if (n > 0)
    {
      totalRead += n;
    }
    else if (n == 0)
    {
      // EOF
      break;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      // No data available, wait a bit
      continue;
    }
    else
    {
      return std::unexpected(std::error_code{errno, std::system_category()});
    }
  }

  return totalRead;
}

std::expected<size_t, std::error_code> serial_port::write(std::span<const uint8_t> data)
{
  if (!is_open())
  {
    return std::unexpected(std::error_code{EBADF, std::system_category()});
  }

  size_t totalWritten = 0;

  while (totalWritten < data.size())
  {
    ssize_t n = ::write(m_fd, data.data() + totalWritten, data.size() - totalWritten);

    if (n > 0)
    {
      totalWritten += n;
    }
    else if (n == 0)
    {
      break;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      continue;
    }
    else
    {
      return std::unexpected(std::error_code{errno, std::system_category()});
    }
  }

  // Wait for data to be transmitted
  tcdrain(m_fd);

  return totalWritten;
}

// read_awaitable implementation
read_awaitable::read_awaitable(serial_port &port, event_loop &loop, std::span<uint8_t> buffer,
                             std::chrono::milliseconds timeout)
    : m_port(port), m_loop(loop), m_buffer(buffer), m_timeout(timeout)
{
}

bool read_awaitable::await_ready() const noexcept
{
  return false; // Always suspend to check for data
}

void read_awaitable::await_suspend(std::coroutine_handle<> handle)
{
  m_handle      = handle;
  m_start_time = std::chrono::steady_clock::now();

  // Try immediate read first
  perform_read();

  if (m_bytes_read >= m_buffer.size() || m_error)
  {
    // Already complete or error
    handle.resume();
    return;
  }

  // Register for read events
  auto result = m_loop.register_fd(m_port.fd(), event_type::read, [this]() { on_readable(); });
  if (!result)
  {
    m_error = result.error();
    handle.resume();
    return;
  }

  // Set up timeout
  auto timerResult = m_loop.create_timer(m_timeout, [this]() { on_timeout(); });
  if (timerResult)
  {
    m_timer_fd = *timerResult;
  }
}

std::expected<size_t, std::error_code> read_awaitable::await_resume()
{
  // Clean up
  m_loop.unregister_fd(m_port.fd());
  if (m_timer_fd >= 0)
  {
    m_loop.cancel_timer(m_timer_fd);
  }

  if (m_error)
  {
    return std::unexpected(m_error);
  }

  return m_bytes_read;
}

void read_awaitable::perform_read()
{
  while (m_bytes_read < m_buffer.size())
  {
    ssize_t n = ::read(m_port.fd(), m_buffer.data() + m_bytes_read, m_buffer.size() - m_bytes_read);

    if (n > 0)
    {
      m_bytes_read += n;
    }
    else if (n == 0)
    {
      // EOF
      break;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      // No more data available now
      break;
    }
    else
    {
      m_error = std::error_code{errno, std::system_category()};
      break;
    }
  }
}

void read_awaitable::on_readable()
{
  perform_read();

  if (m_bytes_read >= m_buffer.size() || m_error)
  {
    if (m_handle)
    {
      m_handle.resume();
    }
  }
}

void read_awaitable::on_timeout()
{
  m_error = std::error_code{ETIMEDOUT, std::system_category()};
  if (m_handle)
  {
    m_handle.resume();
  }
}

// write_awaitable implementation
write_awaitable::write_awaitable(serial_port &port, event_loop &loop, std::span<const uint8_t> data)
    : m_port(port), m_loop(loop), m_data(data)
{
}

bool write_awaitable::await_ready() const noexcept
{
  return false;
}

void write_awaitable::await_suspend(std::coroutine_handle<> handle)
{
  m_handle = handle;

  // Try immediate write
  perform_write();

  if (m_bytes_written >= m_data.size() || m_error)
  {
    handle.resume();
    return;
  }

  // Register for write events
  auto result = m_loop.register_fd(m_port.fd(), event_type::write, [this]() { on_writable(); });
  if (!result)
  {
    m_error = result.error();
    handle.resume();
  }
}

std::expected<size_t, std::error_code> write_awaitable::await_resume()
{
  m_loop.unregister_fd(m_port.fd());

  if (m_error)
  {
    return std::unexpected(m_error);
  }

  // Wait for transmission to complete
  tcdrain(m_port.fd());

  return m_bytes_written;
}

void write_awaitable::perform_write()
{
  while (m_bytes_written < m_data.size())
  {
    ssize_t n = ::write(m_port.fd(), m_data.data() + m_bytes_written, m_data.size() - m_bytes_written);

    if (n > 0)
    {
      m_bytes_written += n;
    }
    else if (n == 0)
    {
      break;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      break;
    }
    else
    {
      m_error = std::error_code{errno, std::system_category()};
      break;
    }
  }
}

void write_awaitable::on_writable()
{
  perform_write();

  if (m_bytes_written >= m_data.size() || m_error)
  {
    if (m_handle)
    {
      m_handle.resume();
    }
  }
}

} // namespace carbio::internal
