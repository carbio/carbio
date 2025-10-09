#include "serial_port_unix.h"

#include "carbio/assert.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <stdexcept>
#include <utility>

#ifdef __linux__
#include <linux/serial.h>
#endif

namespace carbio
{
bool
serial_port::impl::is_open() const noexcept
{
  return fd_.is_valid();
}

bool
serial_port::impl::open(std::string_view path, serial_config config) noexcept
{
  if (is_open()) close();
  config_ = std::move(config);
  fd_.reset(::open(path.data(), O_RDWR | O_NOCTTY));
  if (!is_open())
    return false;
  int flags = ::fcntl(fd_.native_handle(), F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fd_.native_handle(), F_SETFL, flags);
  store_port_state();
  apply_config();
  return true;
}

void
serial_port::impl::close() noexcept
{
  if (!is_open()) return;
  restore_port_state();
  fd_.reset();
}

std::size_t
serial_port::impl::write_some(std::span<const std::uint8_t> buffer) noexcept
{
  if (!is_open()) return 0;
  ssize_t bytes_written{};
  bytes_written = ::write(fd_.native_handle(), buffer.data(), buffer.size());
  return static_cast<std::size_t>(std::max<ssize_t>(bytes_written, 0));
}

std::size_t
serial_port::impl::read_some(std::span<std::uint8_t> buffer) noexcept
{
  if (!is_open()) return 0;
  ssize_t bytes_read{};
  bytes_read = ::read(fd_.native_handle(), buffer.data(), buffer.size());
  return static_cast<std::size_t>(std::max<ssize_t>(bytes_read, 0));
}

std::size_t
serial_port::impl::write_exact(std::span<const std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  if (!is_open()) return 0;

  std::size_t total = 0;
  auto start = std::chrono::steady_clock::now();

  while (total < buffer.size())
  {
    // Calculate remaining timeout
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= timeout) break;

    auto remain_us = std::chrono::duration_cast<std::chrono::microseconds>(timeout - elapsed).count();

    // Wait for write readiness
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd_.native_handle(), &writefds);

    struct timeval tv;
    tv.tv_sec = remain_us / 1000000;
    tv.tv_usec = remain_us % 1000000;

    int ret = ::select(fd_.native_handle() + 1, nullptr, &writefds, nullptr, &tv);
    if (ret <= 0) break; // Timeout or error

    // Write data
    ssize_t n = ::write(fd_.native_handle(), buffer.data() + total, buffer.size() - total);
    if (n > 0)
    {
      total += n;
    }
    else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
    {
      break;
    }
  }

  return total;
}

std::size_t
serial_port::impl::read_exact(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  if (!is_open()) return 0;

  std::size_t total = 0;
  auto start = std::chrono::steady_clock::now();

  while (total < buffer.size())
  {
    // Calculate remaining timeout
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= timeout) break;

    auto remain_us = std::chrono::duration_cast<std::chrono::microseconds>(timeout - elapsed).count();

    // Wait for data availability
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd_.native_handle(), &readfds);

    struct timeval tv;
    tv.tv_sec = remain_us / 1000000;
    tv.tv_usec = remain_us % 1000000;

    int ret = ::select(fd_.native_handle() + 1, &readfds, nullptr, nullptr, &tv);
    if (ret <= 0) break; // Timeout or error

    // Read data
    ssize_t n = ::read(fd_.native_handle(), buffer.data() + total, buffer.size() - total);
    if (n > 0)
    {
      total += n;
    }
    else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
    {
      break;
    }
  }

  return total;
}

std::size_t
serial_port::impl::available() const noexcept
{
  if (!is_open()) return 0;
  int bytes = 0;
  ::ioctl(fd_.native_handle(), FIONREAD, &bytes);
  return bytes >= 0 ? static_cast<std::size_t>(bytes) : 0;
}

void
serial_port::impl::flush() noexcept
{
  if (!is_open()) return ;
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
}

void
serial_port::impl::drain() noexcept
{
  if (!is_open()) return ;
  ::tcdrain(fd_.native_handle());
}

void
serial_port::impl::cancel() noexcept
{
  if (!is_open()) return ;
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
}

void
serial_port::impl::apply_config() noexcept
{
  if (!is_open()) return ;
  init_port_changes();
  apply_baud_rate(config_.baud_rate);
  apply_data_bits(std::to_underlying(config_.data_bits));
  apply_stop_bits(std::to_underlying(config_.stop_bits));
  apply_parity(std::to_underlying(config_.parity));
  apply_flow_control(std::to_underlying(config_.flow));
  apply_timeouts();
  flush();
  apply_port_changes();
}

void
serial_port::impl::apply_baud_rate(std::uint32_t baud_rate) noexcept
{
  if (!is_open()) return;

  speed_t speed = B0;
  bool is_standard = true;

  // Try standard POSIX baud rates first
  switch (baud_rate)
  {
    case 9600:    speed = B9600;    break;
    case 19200:   speed = B19200;   break;
    case 38400:   speed = B38400;   break;
    case 57600:   speed = B57600;   break;
    case 115200:  speed = B115200;  break;
#ifdef B230400
    case 230400:  speed = B230400;  break;
#endif
#ifdef B460800
    case 460800:  speed = B460800;  break;
#endif
#ifdef B500000
    case 500000:  speed = B500000;  break;
#endif
#ifdef B576000
    case 576000:  speed = B576000;  break;
#endif
#ifdef B921600
    case 921600:  speed = B921600;  break;
#endif
#ifdef B1000000
    case 1000000: speed = B1000000; break;
#endif
    default:
      is_standard = false;
      break;
  }

  if (is_standard)
  {
    // Use standard termios API
    ::cfsetospeed(&newtty_, speed);
    ::cfsetispeed(&newtty_, speed);
  }
  else
  {
    // Non-standard baud rate
    bool custom_success = false;

#ifdef __linux__
    // Linux-specific: Try custom baud rate via ioctl
    struct serial_struct serial;
    if (::ioctl(fd_.native_handle(), TIOCGSERIAL, &serial) == 0)
    {
      serial.flags &= ~ASYNC_SPD_MASK;
      serial.flags |= ASYNC_SPD_CUST;
      serial.custom_divisor = serial.baud_base / baud_rate;

      if (::ioctl(fd_.native_handle(), TIOCSSERIAL, &serial) == 0)
      {
        // Set to 38400 as placeholder (Linux driver uses custom_divisor)
        ::cfsetospeed(&newtty_, B38400);
        ::cfsetispeed(&newtty_, B38400);
        custom_success = true;
      }
    }
#endif

    // If custom baud rate failed, fall back to closest standard rate
    if (!custom_success)
    {
      // Default to 57600 for unsupported rates
      ::cfsetospeed(&newtty_, B57600);
      ::cfsetispeed(&newtty_, B57600);
    }
  }
}

void
serial_port::impl::apply_data_bits(std::uint8_t data_bits) noexcept
{
  if (!is_open()) return ;

  /* set data bits */
  switch (data_bits)
    {
      /* 5 bit */
      case 5:
        {
          newtty_.c_cflag &= ~CSIZE;
          newtty_.c_cflag |= CS5;
          break;
        }

      /* 6 bit */
      case 6:
        {
          newtty_.c_cflag &= ~CSIZE;
          newtty_.c_cflag |= CS6;
          break;
        }

      /* 7 bit */
      case 7:
        {
          newtty_.c_cflag &= ~CSIZE;
          newtty_.c_cflag |= CS7;
          break;
        }

      /* 8 bit */
      case 8:
        {
          newtty_.c_cflag &= ~CSIZE;
          newtty_.c_cflag |= CS8;
          break;
        }

      /* invalid param */
      default:
        {
          throw std::runtime_error("invalid data width setting");
        }
    }
}

void
serial_port::impl::apply_stop_bits(std::uint8_t stop_bits) noexcept
{
  if (!is_open()) return ;

  /* set stop bits */
  switch (stop_bits)
    {
      /* 1 stop bit */
      case 1:
        {
          newtty_.c_cflag &= ~CSTOPB;
          break;
        }

      /* 2 stop bits */
      case 2:
        {
          newtty_.c_cflag |= CSTOPB;
          break;
        }

      /* invalid param */
      default:
        {
          throw std::runtime_error("invalid stop width setting");
        }
    }
}

void
serial_port::impl::apply_parity(std::uint8_t parity) noexcept
{
  if (!is_open()) return ;

  /* set parity bits */
  switch (parity)
    {
      /* none */
      case 1:
        {
          newtty_.c_cflag &= ~PARENB;
          newtty_.c_iflag &= ~INPCK;
          break;
        }

      /* odd */
      case 2:
        {
          newtty_.c_cflag |= (PARODD | PARENB);
          newtty_.c_iflag |= INPCK;
          break;
        }

      /* even */
      case 3:
        {
          newtty_.c_cflag |= PARENB;
          newtty_.c_cflag &= ~PARODD;
          newtty_.c_iflag |= INPCK;
          break;
        }

      /* invalid */
      default:
        {
          throw std::runtime_error("invalid parity setting");
        }
    }
}

void
serial_port::impl::apply_flow_control(std::uint8_t flow) noexcept
{
  if (!is_open()) return ;

  /* set flow control */
  switch (flow)
    {
      /* none */
      case 0:
        {
          newtty_.c_cflag &= ~CRTSCTS;
          newtty_.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
          break;
        }

      /* software */
      case 1:
        {
          newtty_.c_cflag &= ~CRTSCTS;
          newtty_.c_iflag |= (IXON | IXOFF);
          break;
        }

      /* hardware */
      case 2:
        {
          newtty_.c_cflag |= CRTSCTS;
          newtty_.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
          break;
        }

      /* both */
      case 3:
        {
          newtty_.c_cflag |= CRTSCTS;
          newtty_.c_iflag |= (IXON | IXOFF);
          break;
        }

      /* invalid */
      default:
        {
          throw std::runtime_error("invalid flow control setting");
        }
    }
}

void
serial_port::impl::apply_timeouts() noexcept
{
  if (!is_open()) return ;
  newtty_.c_cc[VMIN]  = 1; /* 0=timeout, 1=return each byte */
  newtty_.c_cc[VTIME] = 0; /* 0=nonblocking read, 1=blocking read  */
}

void
serial_port::impl::store_port_state() noexcept
{
  if (!is_open()) return ;
  ::tcgetattr(fd_.native_handle(), &oldtty_);
}

void
serial_port::impl::restore_port_state() noexcept
{
  if (!is_open()) return ;
  ::tcsetattr(fd_.native_handle(), TCSANOW, &oldtty_);
}

void
serial_port::impl::init_port_changes() noexcept
{
  if (!is_open()) return ;
  ::tcgetattr(fd_.native_handle(), &newtty_);
  cfmakeraw(&newtty_);

  // Enable receiver and ignore modem control lines
  // cfmakeraw() does NOT set these - they must be set explicitly
  newtty_.c_cflag |= (CREAD | CLOCAL);
}

void
serial_port::impl::apply_port_changes() noexcept
{
  if (!is_open()) return ;
  ::tcsetattr(fd_.native_handle(), TCSANOW, &newtty_);
}
} // namespace carbio