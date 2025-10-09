#include "serial_port_unix.h"

#include "carbio/assert.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>

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
  if (is_open())
    close();
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
  if (!is_open())
    {
      return;
    }
  restore_port_state();
  fd_.reset();
}

std::size_t
serial_port::impl::write_some(std::span<const std::uint8_t> buffer) noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  ssize_t bytes_written{};
  bytes_written = ::write(fd_.native_handle(), buffer.data(), buffer.size());
  return static_cast<std::size_t>(std::max<ssize_t>(bytes_written, 0));
}

std::size_t
serial_port::impl::read_some(std::span<std::uint8_t> buffer) noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  ssize_t bytes_read{};
  bytes_read = ::read(fd_.native_handle(), buffer.data(), buffer.size());
  return static_cast<std::size_t>(std::max<ssize_t>(bytes_read, 0));
}

std::size_t
serial_port::impl::write_exact(std::span<const std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  std::size_t total = 0;
  auto        start = std::chrono::steady_clock::now();

  while (total < buffer.size())
    {
      if (std::chrono::steady_clock::now() - start >= timeout)
        {
          break;
        }

      ssize_t n = ::write(fd_.native_handle(), buffer.data() + total, buffer.size() - total);
      if (n > 0)
        {
          total += n;
        }
      else if (n < 0 && errno != EINTR)
        {
          break;
        }
    }

  return total;
}

std::size_t
serial_port::impl::read_exact(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");

  std::size_t total = 0;
  auto        start = std::chrono::steady_clock::now();

  while (total < buffer.size())
    {
      if (std::chrono::steady_clock::now() - start >= timeout)
        {
          break;
        }

      ssize_t n = ::read(fd_.native_handle(), buffer.data() + total, buffer.size() - total);
      if (n > 0)
        {
          total += n;
        }
      else if (n < 0 && errno != EINTR)
        {
          break;
        }
    }

  return total;
}

std::size_t
serial_port::impl::available() const noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  int bytes = 0;
  ::ioctl(fd_.native_handle(), FIONREAD, &bytes);
  return bytes >= 0 ? static_cast<std::size_t>(bytes) : 0;
}

void
serial_port::impl::flush() noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
}

void
serial_port::impl::drain() noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  ::tcdrain(fd_.native_handle());
}

void
serial_port::impl::cancel() noexcept
{
  ct_panic_msg(fd_.is_valid(), "port not open");
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
}

void
serial_port::impl::apply_config() noexcept
{
  if (!is_open())
    return;
  init_port_changes();
  apply_baud(config_.baud);
  apply_data_bits(std::to_underlying(config_.data_bits));
  apply_stop_bits(std::to_underlying(config_.stop_bits));
  apply_parity(std::to_underlying(config_.parity));
  apply_flow_control(std::to_underlying(config_.flow));
  apply_timeouts();
  flush();
  apply_port_changes();
}

void
serial_port::impl::apply_baud(std::uint32_t baud) noexcept
{
  if (0 != ::cfsetospeed(&newtty_, (speed_t)baud))
    perror("serial: set output speed failed.\n");
  if (0 != ::cfsetispeed(&newtty_, (speed_t)baud))
    perror("serial: set input speed failed.\n");
}

void
serial_port::impl::apply_data_bits(std::uint8_t data_bits) noexcept
{
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
          perror("serial: data bits is invalid.\n");
          break;
        }
    }
}

void
serial_port::impl::apply_stop_bits(std::uint8_t stop_bits) noexcept
{
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
          perror("serial: stop bits is invalid.\n");
          break;
        }
    }
}

void
serial_port::impl::apply_parity(std::uint8_t parity) noexcept
{
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
          perror("serial: parity is invalid.\n");
          break;
        }
    }
}

void
serial_port::impl::apply_flow_control(std::uint8_t flow) noexcept
{
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
          perror("serial: invalid flow control.\n");
          break;
        }
    }
}

void
serial_port::impl::apply_timeouts() noexcept
{
  newtty_.c_cc[VMIN]  = 1; /* 0=timeout, 1=return each byte */
  newtty_.c_cc[VTIME] = 0; /* 0=nonblocking read, 1=blocking read  */
}

void
serial_port::impl::store_port_state() noexcept
{
  ::tcgetattr(fd_.native_handle(), &oldtty_);
}

void
serial_port::impl::restore_port_state() noexcept
{
  ::tcsetattr(fd_.native_handle(), TCSANOW, &oldtty_);
}

void
serial_port::impl::init_port_changes() noexcept
{
  ::tcgetattr(fd_.native_handle(), &newtty_);
  cfmakeraw(&newtty_);
}

void
serial_port::impl::apply_port_changes() noexcept
{

  ::tcsetattr(fd_.native_handle(), TCSANOW, &newtty_);
}
} // namespace carbio