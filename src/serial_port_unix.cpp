#include "serial_port_unix.h"

#include "carbio/assert.h"
#include <spdlog/spdlog.h>

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
serial_port::impl::open(std::string_view path) noexcept
{
  spdlog::info("opening port at {}", path);
  if (is_open())
  {
    spdlog::warn("port already open, closing port first");
    close();
  }
  
  /* open non-blocking serial */
  fd_.reset(::open(path.data(), O_RDWR | O_NOCTTY | O_NONBLOCK));
  if (!is_open())
  {
    spdlog::error("could not open port {}: {}", path, strerror(errno));
    return false;
  }

  /* store current settings */
  if (::tcgetattr(fd_.native_handle(), &oldtty_) != 0)
  {
    spdlog::error("Failed to get port attributes: {}", strerror(errno));
    close();
    return false;
  }

  /* initialize new settings from current state */
  newtty_ = oldtty_; 

  /* configure for raw mode */
  cfmakeraw(&newtty_);

  /* disable input processing */
  /* flow control */
  /* IXON: disable xon/xoff output flow control */
  /* IXOFF: disable xon/xoff input flow control */
  /* IXANY: don't allow any character to restart output */
  newtty_.c_iflag &= ~(IXON | IXOFF | IXANY);
  
  /* break handling */
  /* IGNBRK: don't ignore break conditions */
  /* BRKINT: don't send sigint signal on break */
  /* PARMRK: don't mark parity errors */
  newtty_.c_iflag &= ~(IGNBRK | BRKINT | PARMRK);
  
  /* character processing */
  /* ISTRIP: don't strip 8th bit */
  /* INLCR: don't translate newline to carriage return */
  /* IGNCR: don't ignore carriage return */
  /* ICRNL: don't translate carriage return to newline */
  newtty_.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL);
  
  /* disable all output post-processing */
  /* no cr->nl conversion */
  /* no tab expansion */
  /* no delay insertions */
  newtty_.c_oflag &= ~OPOST;
  
  /* disable terminal line editing */
  /* ECHO: don't echo received characters back */
  /* ECHONL: don't echo newlines */
  /* ECHOE: don't echo erase characters (backspace) */
  /* ICANON: disable canonical mode */
  /* ISIG: don't generate signals */
  /* IEXTEN: disable extended input processing */
  newtty_.c_lflag &= ~(ECHO | ECHONL | ECHOE | ICANON | ISIG | IEXTEN);
  
  /* enable essential hardware settings */
  /* CLOCAL: ignore modem control lines */
  /* CREAD: enable receiver (allow reading data) */
  newtty_.c_cflag |= (CLOCAL | CREAD);
  
  /* non-blocking reads */
  newtty_.c_cc[VTIME] = 0; /* 0=nonblocking read, 1=blocking read  */
  newtty_.c_cc[VMIN] = 0;  /* 0=timeout, 1=return each byte */

  // Baud rate will be set explicitly by caller after open()
  // Set safe defaults for other parameters
  set_data_width(data_width::_8);
  set_stop_width(stop_width::_1);
  set_parity_mode(parity_mode::none);
  set_flow_control(flow_control::none);

  flush();
  spdlog::info("port open at {}", path);
  return true;
}

bool
serial_port::impl::set_baud_rate(std::uint32_t baud) noexcept
{
  spdlog::info("setting baud rate {}", baud);

  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }

  speed_t speed = B0;
  switch (baud)
  {
    /* 9600bps */
    case 9600:
    {
      speed = B9600;
      break;
    }

    /* 19200bps */
    case 19200:
    {
      speed = B19200;
      break;
    }

    /* 38400bps */
    case 38400:
    {
      speed = B38400;
      break;
    }

    /* 57600bps */
    case 57600:
    {
      speed = B57600;
      break;
    }

    /* 115200bps */
    case 115200:
    {
      speed = B115200;
      break;
    }

    /* invalid */
    default:
    {
      spdlog::error("non-standard posix baud-rate {}", baud);
      return false;
    }
  }

  if (0 != ::cfsetospeed(&newtty_, speed))
  {
    spdlog::error("cfsetospeed({}, ...) failed: {}", baud, std::strerror(errno));
    return false;
  }

  if (0 != ::cfsetispeed(&newtty_, speed))
  {
    spdlog::error("cfsetispeed({}, ...) failed: {}", baud, std::strerror(errno));
    return false;
  }

  spdlog::info("baud rate set");
  return apply_port_settings();
}

bool
serial_port::impl::set_data_width(data_width data) noexcept
{
  spdlog::info("setting data width {}", static_cast<std::uint8_t>(data));

  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }

  /* set data width */
  switch (data)
  {
    /* 5 bit */
    case data_width::_5:
    {
      newtty_.c_cflag &= ~CSIZE;
      newtty_.c_cflag |= CS5;
      break;
    }

    /* 6 bit */
    case data_width::_6:
    {
      newtty_.c_cflag &= ~CSIZE;
      newtty_.c_cflag |= CS6;
      break;
    }

    /* 7 bit */
    case data_width::_7:
    {
      newtty_.c_cflag &= ~CSIZE;
      newtty_.c_cflag |= CS7;
      break;
    }

    /* 8 bit */
    case data_width::_8:
    {
      newtty_.c_cflag &= ~CSIZE;
      newtty_.c_cflag |= CS8;
      break;
    }

    /* invalid */
    default:
    {
      spdlog::error("data width is invalid");
      return false;
    }
  }

  spdlog::info("data width is set");
  return apply_port_settings();
}

bool
serial_port::impl::set_stop_width(stop_width stop) noexcept
{
  spdlog::info("setting stop width {}", static_cast<std::uint8_t>(stop));
  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }

  /* set stop width */
  switch (stop)
  {
    /* 1 stop bit */
    case stop_width::_1:
    {
      newtty_.c_cflag &= ~CSTOPB;
      break;
    }

    /* 2 stop bits */
    case stop_width::_2:
    {
      newtty_.c_cflag |= CSTOPB;
      break;
    }

    /* invalid */
    default:
    {
      spdlog::error("stop width is invalid");
      return false;
    }
  }

  spdlog::info("stop width is set");
  return apply_port_settings();
}

bool
serial_port::impl::set_parity_mode(parity_mode parity) noexcept
{
  spdlog::info("setting parity mode {}", static_cast<std::uint8_t>(parity));

  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }

  /* set parity width */
  switch (parity)
  {
    /* none */
    case parity_mode::none:
    {
      newtty_.c_cflag &= ~PARENB;
      newtty_.c_iflag &= ~INPCK;
      break;
    }

    /* odd */
    case parity_mode::odd:
    {
      newtty_.c_cflag |= (PARODD | PARENB);
      newtty_.c_iflag |= INPCK;
      break;
    }

    /* even */
    case parity_mode::even:
    {
      newtty_.c_cflag |= PARENB;
      newtty_.c_cflag &= ~PARODD;
      newtty_.c_iflag |= INPCK;
      break;
    }

    /* invalid */
    default:
    {
      spdlog::error("parity mode is invalid");
      return false;
    }
  }

  spdlog::info("parity mode is set");
  return apply_port_settings();
}

bool
serial_port::impl::set_flow_control(flow_control flow) noexcept
{
  spdlog::info("setting flow control {}", static_cast<std::uint8_t>(flow));

  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }

  /* set flow control */
  switch (flow)
  {
    /* none */
    case flow_control::none:
    {
      newtty_.c_cflag &= ~CRTSCTS;
      newtty_.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
      break;
    }

    /* software */
    case flow_control::software:
    {
      newtty_.c_cflag &= ~CRTSCTS;
      newtty_.c_iflag |= (IXON | IXOFF);
      break;
    }

    /* hardware */
    case flow_control::hardware:
    {
      newtty_.c_cflag |= CRTSCTS;
      newtty_.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
      break;
    }

    /* both */
    case flow_control::both:
    {
      newtty_.c_cflag |= CRTSCTS;
      newtty_.c_iflag |= (IXON | IXOFF);
      break;
    }

    /* invalid */
    default:
    {
      spdlog::error("flow control invalid");
      return false;
    }
  }

  spdlog::info("flow control set");
  return apply_port_settings();
}

//bool
//serial_port::impl::set_blocking(bool value) noexcept
//{
//  spdlog::info("setting blocking mode {}", value);
//  newtty_.c_cc[VTIME] = value; /* 0=nonblocking read, 1=blocking read  */
//  newtty_.c_cc[VMIN]  = 1;    
//  spdlog::info("blocking mode set.");
//  return true;
//}

void
serial_port::impl::close() noexcept
{
  spdlog::info("closing port...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return;
  }
  ::tcsetattr(fd_.native_handle(), TCSANOW, &oldtty_);
  fd_.reset();
  spdlog::info("port closed");
}

std::size_t
serial_port::impl::write_some(std::span<const std::uint8_t> buffer) noexcept
{
  if (!is_open())
  {
    spdlog::warn("port not open");
    return 0;
  }
  ssize_t bytes_written = ::write(fd_.native_handle(), buffer.data(), buffer.size());
  if (0 > bytes_written)
  {
    if (errno == EAGAIN)
    {
      if (spdlog::should_log(spdlog::level::trace))
        spdlog::trace("write would block");
      return 0;
    }
    spdlog::error("write failed {}", strerror(errno));
    return 0;
  }
  if (spdlog::should_log(spdlog::level::trace))
    spdlog::debug("written {} bytes", bytes_written);
  return std::max<std::size_t>(0, bytes_written);
}

std::size_t
serial_port::impl::read_some(std::span<std::uint8_t> buffer) noexcept
{
  if (!is_open())
  {
    spdlog::warn("port not open");
    return 0;
  }
  const auto bytes_read = ::read(fd_.native_handle(), buffer.data(), buffer.size());
  if (0 > bytes_read)
  {
    if (errno == EAGAIN)
    {
      if (spdlog::should_log(spdlog::level::trace))
        spdlog::trace("no data available");
      return 0;
    }
    spdlog::error("read failed {}", strerror(errno));
    return 0;
  }
  if (spdlog::should_log(spdlog::level::trace))
    spdlog::debug("read {} bytes", bytes_read);
  return std::max<std::size_t>(0, bytes_read);
}

namespace {
[[gnu::noinline]]
int wait_for_read(int fd, std::int64_t timeout_us) noexcept
{
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);

  struct timeval tv{};
  tv.tv_sec  = timeout_us / 1000000;
  tv.tv_usec = timeout_us % 1000000;

  return ::select(fd + 1, &readfds, nullptr, nullptr, &tv);
}

[[gnu::noinline]]
int wait_for_write(int fd, std::int64_t timeout_us) noexcept
{
  fd_set writefds;
  FD_ZERO(&writefds);
  FD_SET(fd, &writefds);

  struct timeval tv{};
  tv.tv_sec  = timeout_us / 1000000;
  tv.tv_usec = timeout_us % 1000000;

  return ::select(fd + 1, nullptr, &writefds, nullptr, &tv);
}

[[gnu::noinline]]
std::size_t do_read_exact(int fd, std::span<std::uint8_t> buffer, std::int64_t timeout_us) noexcept
{
  std::size_t total_bytes = 0;
  const auto start_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  while (total_bytes < buffer.size())
  {
    const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
    const auto elapsed_us = now_us - start_us;
    if (elapsed_us >= timeout_us) break;

    const auto select_result = wait_for_read(fd, timeout_us - elapsed_us);
    if (select_result <= 0) break;

    const auto read_bytes = ::read(fd, buffer.data() + total_bytes, buffer.size() - total_bytes);
    if (0 > read_bytes && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) break;
    if (0 < read_bytes) total_bytes += static_cast<std::size_t>(read_bytes);
  }

  return total_bytes;
}

[[gnu::noinline]]
std::size_t do_write_exact(int fd, std::span<const std::uint8_t> buffer, std::int64_t timeout_us) noexcept
{
  std::size_t total_bytes = 0;
  const auto start_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  while (total_bytes < buffer.size())
  {
    const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
    const auto elapsed_us = now_us - start_us;
    if (elapsed_us >= timeout_us) break;

    const auto select_result = wait_for_write(fd, timeout_us - elapsed_us);
    if (select_result <= 0) break;

    const auto bytes_written = ::write(fd, buffer.data() + total_bytes, buffer.size() - total_bytes);
    if (bytes_written < 0)
    {
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) break;
      continue;
    }
    total_bytes += static_cast<std::size_t>(bytes_written);
  }
  
  return total_bytes;
}
} // anonymous namespace

std::size_t
serial_port::impl::write_exact(std::span<const std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  if (!is_open())
  {
    spdlog::warn("port not open");
    return 0;
  }

  const auto timeout_us = timeout.count() * 1000;
  const auto total_bytes = do_write_exact(fd_.native_handle(), buffer, timeout_us);

  if (spdlog::should_log(spdlog::level::trace))
    spdlog::trace("write completed {}/{} bytes", total_bytes, buffer.size());
  return total_bytes;
}

std::size_t
serial_port::impl::read_exact(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  if (!is_open())
  {
    spdlog::warn("port not open");
    return 0;
  }

  const auto timeout_us = timeout.count() * 1000;
  const auto total_bytes = do_read_exact(fd_.native_handle(), buffer, timeout_us);

  if (spdlog::should_log(spdlog::level::trace))
    spdlog::trace("read completed {}/{} bytes", total_bytes, buffer.size());
  return total_bytes;
}

std::size_t
serial_port::impl::available() const noexcept
{
  if (!is_open())
  {
    spdlog::warn("port not open");
    return 0;
  }
  int bytes = 0;
  ::ioctl(fd_.native_handle(), FIONREAD, &bytes);
  if (spdlog::should_log(spdlog::level::trace))
    spdlog::trace("available {} bytes", bytes);
  return static_cast<std::size_t>(std::max(0, bytes));
}

void
serial_port::impl::flush() noexcept
{
  spdlog::trace("flushing...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return;
  }
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
  spdlog::trace("flushed");
}

void
serial_port::impl::drain() noexcept
{
  spdlog::trace("draining...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return;
  }
  ::tcdrain(fd_.native_handle());
  spdlog::trace("drained");
}

void
serial_port::impl::cancel() noexcept
{
  spdlog::trace("cancelling...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return;
  }
  ::tcflush(fd_.native_handle(), TCIOFLUSH);
  spdlog::trace("cancelled");
}

bool
serial_port::impl::apply_port_settings() noexcept
{
  spdlog::debug("applying port settings...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }
  if (0 != ::tcsetattr(fd_.native_handle(), TCSANOW, &newtty_))
  {
    spdlog::error("could not apply port changes: {}", strerror(errno));
    return false;
  }
  spdlog::debug("port settings applied");
  return true;
}

bool
serial_port::impl::restore_port_settings() noexcept
{
  spdlog::debug("restoring port settings...");
  if (!is_open())
  {
    spdlog::warn("port not open");
    return false;
  }
  if (0 != ::tcsetattr(fd_.native_handle(), TCSANOW, &oldtty_))
  {
    spdlog::error("could not restore port changes: {}", strerror(errno));
    return false;
  }
  spdlog::debug("port settings restored");
  return true;
}

} // namespace carbio