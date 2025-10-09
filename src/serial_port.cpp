#include "carbio/serial_port.h"
#if defined(__unix) || defined(__unix__) || defined(unix)
#include "serial_port_unix.h"
#else
#error This platform is not supported.
#endif
#include "carbio/assert.h"

namespace carbio
{
serial_port::serial_port() noexcept : d(std::make_unique<impl>())
{
  ct_panic(nullptr != d);
}

serial_port::~serial_port() noexcept = default;

serial_port::serial_port(serial_port &&other) noexcept = default;

serial_port &
serial_port::operator=(serial_port &&other) noexcept = default;

bool
serial_port::is_open() const noexcept
{
  ct_panic(nullptr != d);
  return d->is_open();
}

bool
serial_port::open(std::string_view device_path, serial_config config) noexcept
{
  ct_panic(nullptr != d);
  return d->open(device_path, std::move(config));
}

void
serial_port::close() noexcept
{
  ct_panic(nullptr != d);
  d->close();
}

std::size_t
serial_port::write_some(std::span<const std::uint8_t> buffer) noexcept
{
  ct_panic(nullptr != d);
  return d->write_some(buffer);
}

std::size_t
serial_port::read_some(std::span<std::uint8_t> buffer) noexcept
{
  ct_panic(nullptr != d);
  return d->read_some(buffer);
}

std::size_t
serial_port::write_exact(std::span<const std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  ct_panic(nullptr != d);
  return d->write_exact(buffer, timeout);
}

std::size_t
serial_port::read_exact(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) noexcept
{
  ct_panic(nullptr != d);
  return d->read_exact(buffer, timeout);
}

std::size_t
serial_port::available() const noexcept
{
  ct_panic(nullptr != d);
  return d->available();
}

void
serial_port::flush() noexcept
{
  ct_panic(nullptr != d);
  d->flush();
}

void
serial_port::drain() noexcept
{
  ct_panic(nullptr != d);
  d->drain();
}

void
serial_port::cancel() noexcept
{
  ct_panic(nullptr != d);
  d->cancel();
}
} /* namespace carbio */
