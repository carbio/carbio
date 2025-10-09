#include "carbio/fingerprint.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace carbio
{

// Anonymous namespace for helper functions
namespace {

using internal::read_be;

} // anonymous namespace

fingerprint_sensor::fingerprint_sensor(internal::event_loop &loop, std::string_view device_path)
    : m_loop(loop),
      m_serial(loop),
      m_protocol(m_serial, default_address),
      m_port_name(device_path),
      m_password(0x00000000),
      m_address(default_address)
{
}

fingerprint_sensor::~fingerprint_sensor()
{
  disconnect();
}

fingerprint_sensor::fingerprint_sensor(fingerprint_sensor &&other) noexcept
    : m_loop(other.m_loop),
      m_serial(m_loop),  // Construct new serial_port (can't move)
      m_protocol(m_serial, other.m_address),  // Construct new protocol_handler (can't move)
      m_port_name(std::move(other.m_port_name)),
      m_password(other.m_password),
      m_address(other.m_address),
      m_cached_params(other.m_cached_params)
{
  other.m_address = default_address;
}

fingerprint_sensor &fingerprint_sensor::operator=(fingerprint_sensor &&other) noexcept
{
  if (this != &other)
  {
    disconnect();

    // serial_port and protocol_handler can't be move-assigned
    // They will remain in their current state
    m_port_name     = std::move(other.m_port_name);
    m_password      = other.m_password;
    m_address       = other.m_address;
    m_cached_params = other.m_cached_params;

    other.m_address = default_address;
  }
  return *this;
}

// ========================================
// PRIVATE - Low-level sensor operations
// ========================================

internal::task<status_code> fingerprint_sensor::verify_password()
{
  std::array<std::uint8_t, 4> pwd{static_cast<std::uint8_t>((m_password >> 24) & 0xFFu),
                                   static_cast<std::uint8_t>((m_password >> 16) & 0xFFu),
                                   static_cast<std::uint8_t>((m_password >> 8) & 0xFFu),
                                   static_cast<std::uint8_t>(m_password & 0xFFu)};

  auto status = co_await send_command(operation_code::verify_device_password, pwd);
  co_return status;
}

internal::task<std::optional<device_setting_info>> fingerprint_sensor::get_parameters()
{
  auto result = co_await send_command_with_data(operation_code::read_system_parameter, {});

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  auto const &d = result.value();
  if (d.size() < 17)
  {
    co_return std::nullopt;
  }

  // Skip first byte (status), parse parameters starting at index 1
  device_setting_info info;
  info.status         = static_cast<uint16_t>((d[1] << 8) | d[2]);
  info.id             = static_cast<uint16_t>((d[3] << 8) | d[4]);
  info.capacity       = static_cast<uint16_t>((d[5] << 8) | d[6]);
  info.security_level = static_cast<uint16_t>((d[7] << 8) | d[8]);
  info.address        = static_cast<uint32_t>((d[9] << 24) | (d[10] << 16) | (d[11] << 8) | d[12]);
  info.length         = static_cast<uint16_t>((d[13] << 8) | d[14]);
  info.baudrate       = static_cast<uint16_t>((d[15] << 8) | d[16]);

  m_cached_params = info;
  co_return info;
}

internal::task<status_code> fingerprint_sensor::capture_image()
{
  co_return co_await send_command(operation_code::capture_image);
}

internal::task<status_code> fingerprint_sensor::image_to_template(uint8_t buffer_slot)
{
  std::array<uint8_t, 1> const data = {buffer_slot};
  co_return co_await send_command(operation_code::extract_features, data);
}

internal::task<status_code> fingerprint_sensor::create_model()
{
  co_return co_await send_command(operation_code::generate_model);
}

internal::task<status_code> fingerprint_sensor::store_template(uint16_t template_id, uint8_t buffer_slot)
{
  std::array<uint8_t, 3> const data = {buffer_slot, static_cast<uint8_t>((template_id >> 8) & 0xFF),
                                        static_cast<uint8_t>(template_id & 0xFF)};
  co_return co_await send_command(operation_code::store_model, data);
}

internal::task<std::optional<search_query_info>> fingerprint_sensor::search(uint8_t buffer_slot, uint16_t start_id,
                                                                         uint16_t count)
{
  if (count == 0)
  {
    count = m_cached_params ? m_cached_params->capacity : 127;
  }

  std::array<uint8_t, 5> const data = {buffer_slot, static_cast<uint8_t>((start_id >> 8) & 0xFF),
                                        static_cast<uint8_t>(start_id & 0xFF), static_cast<uint8_t>((count >> 8) & 0xFF),
                                        static_cast<uint8_t>(count & 0xFF)};

  auto result = co_await send_command_with_data(operation_code::search_model, data);

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  auto const &d = result.value();
  if (d.size() < 5)
  {
    co_return std::nullopt;
  }

  search_query_info search_result;
  search_result.index      = static_cast<uint16_t>((d[1] << 8) | d[2]);
  search_result.confidence = static_cast<uint16_t>((d[3] << 8) | d[4]);

  co_return search_result;
}

internal::task<std::optional<search_query_info>> fingerprint_sensor::fast_search()
{
  // Refresh system parameters before search
  co_await get_parameters();

  uint16_t capacity = m_cached_params ? m_cached_params->capacity : 127;
  if (capacity == 0)
  {
    capacity = 127; // Safe default
  }

  std::array<uint8_t, 5> const data = {0x01, // Buffer slot 1
                                        0x00, // Start ID high byte
                                        0x00, // Start ID low byte
                                        static_cast<uint8_t>((capacity >> 8) & 0xFF),
                                        static_cast<uint8_t>(capacity & 0xFF)};

  auto result = co_await send_command_with_data(operation_code::fast_search, data);

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  auto const &d = result.value();
  if (d.size() < 5)
  {
    co_return std::nullopt;
  }

  search_query_info search_result;
  search_result.index      = static_cast<uint16_t>((d[1] << 8) | d[2]);
  search_result.confidence = static_cast<uint16_t>((d[3] << 8) | d[4]);

  co_return search_result;
}

internal::task<status_code> fingerprint_sensor::delete_template(uint16_t start_id, uint16_t count)
{
  std::array<uint8_t, 4> const data = {static_cast<uint8_t>((start_id >> 8) & 0xFF),
                                        static_cast<uint8_t>(start_id & 0xFF), static_cast<uint8_t>((count >> 8) & 0xFF),
                                        static_cast<uint8_t>(count & 0xFF)};
  co_return co_await send_command(operation_code::erase_model, data);
}

internal::task<status_code> fingerprint_sensor::clear_database()
{
  co_return co_await send_command(operation_code::erase_database);
}

internal::task<std::optional<uint16_t>> fingerprint_sensor::get_template_count()
{
  auto result = co_await send_command_with_data(operation_code::count_model, {});

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  auto const &d = result.value();
  if (d.size() < 3)
  {
    co_return std::nullopt;
  }

  co_return static_cast<uint16_t>((d[1] << 8) | d[2]);
}

internal::task<std::optional<std::vector<uint16_t>>> fingerprint_sensor::fetch_templates()
{
  std::vector<uint16_t> templates;

  // Get system parameters to know library size
  auto params = co_await get_parameters();
  if (!params)
  {
    co_return std::nullopt;
  }

  uint16_t library_size = m_cached_params ? m_cached_params->capacity : 127;
  uint16_t num_pages    = (library_size + 255) / 256;

  for (uint16_t page = 0; page < num_pages; ++page)
  {
    std::array<uint8_t, 1> const data   = {static_cast<uint8_t>(page)};
    auto                         result = co_await send_command_with_data(operation_code::read_index_table, data);

    if (!result.has_value())
    {
      co_return std::nullopt;
    }

    auto const &d = result.value();
    if (d.size() < 33)
    {
      continue;
    }

    // Parse bitmap starting at index 1 (skip status byte)
    for (size_t i = 1; i <= 32; ++i)
    {
      uint8_t byte = d[i];

      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        if (byte & (1 << bit))
        {
          uint16_t templateId = (page * 256) + ((i - 1) * 8) + bit;
          if (templateId < library_size)
          {
            templates.push_back(templateId);
          }
        }
      }
    }
  }

  co_return templates;
}

internal::task<status_code> fingerprint_sensor::control_led(led_mode_setting mode, uint8_t speed, led_color_setting color, uint8_t cycles)
{
  std::array<uint8_t, 4> const data = {static_cast<uint8_t>(mode), speed, static_cast<uint8_t>(color), cycles};
  co_return co_await send_command(operation_code::led_config, data);
}

internal::task<status_code> fingerprint_sensor::turn_led_on()
{
  co_return co_await send_command(operation_code::led_on);
}

internal::task<status_code> fingerprint_sensor::turn_led_off()
{
  co_return co_await send_command(operation_code::led_off);
}

internal::task<status_code> fingerprint_sensor::set_baud_rate(baud_rate_setting baud)
{
  std::array<uint8_t, 2> const data   = {static_cast<uint8_t>(device_setting_index::baud_rate_setting), static_cast<uint8_t>(baud)};
  auto                         status = co_await send_command(operation_code::write_system_parameter, data);

  if (status == status_code::success)
  {
    // Refresh system parameters to update cached values
    co_await get_parameters();
  }

  co_return status;
}

internal::task<status_code> fingerprint_sensor::set_security_level_register(security_level_setting level)
{
  std::array<uint8_t, 2> const data   = {static_cast<uint8_t>(device_setting_index::security_level_setting), static_cast<uint8_t>(level)};
  auto                         status = co_await send_command(operation_code::write_system_parameter, data);

  if (status == status_code::success)
  {
    co_await get_parameters();
  }

  co_return status;
}

internal::task<status_code> fingerprint_sensor::set_data_packet_size_register(packet_length_setting packet_size)
{
  std::array<uint8_t, 2> const data   = {static_cast<uint8_t>(device_setting_index::packet_length_setting), static_cast<uint8_t>(packet_size)};
  auto                         status = co_await send_command(operation_code::write_system_parameter, data);

  if (status == status_code::success)
  {
    co_await get_parameters();
  }

  co_return status;
}

internal::task<status_code> fingerprint_sensor::soft_reset()
{
  auto status = co_await send_command(operation_code::soft_reset);

  if (status == status_code::success)
  {
    // Wait for sensor to stabilize
    co_await internal::sleep_awaitable{m_loop, std::chrono::milliseconds{500}};
    co_await get_parameters();
  }

  co_return status;
}

internal::task<result<std::vector<uint8_t>>> fingerprint_sensor::send_command_with_data(operation_code cmd,
                                                                                  std::span<const uint8_t> data)
{
  // Delegate to protocol_handler - it handles everything!
  co_return co_await m_protocol.send_command(cmd, data, std::chrono::milliseconds{default_timeout_ms});
}

internal::task<status_code> fingerprint_sensor::send_command(operation_code cmd, std::span<const uint8_t> data)
{
  auto result = co_await send_command_with_data(cmd, data);
  if (!result.has_value())
  {
    co_return result.error();
  }
  co_return status_code::success;
}

internal::task<status_code> fingerprint_sensor::load_template(uint16_t template_id, uint8_t buffer_slot)
{
  std::array<uint8_t, 3> const data = {buffer_slot, static_cast<uint8_t>((template_id >> 8) & 0xFF),
                                        static_cast<uint8_t>(template_id & 0xFF)};
  co_return co_await send_command(operation_code::load_model, data);
}

internal::task<std::optional<std::vector<uint8_t>>> fingerprint_sensor::download_template(uint8_t buffer_slot)
{
  std::array<uint8_t, 1> const cmd_data = {buffer_slot};
  auto                         result  = co_await send_command_with_data(operation_code::upload_model, cmd_data);

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  // Delegate to protocol_handler for multi-packet data transfer
  size_t packet_size = m_cached_params ? m_cached_params->length : 128;
  auto data_result = co_await m_protocol.receive_data_packets(std::chrono::milliseconds{default_timeout_ms});
  if (!data_result.has_value())
  {
    co_return std::nullopt;
  }

  co_return data_result.value();
}

internal::task<status_code> fingerprint_sensor::upload_template(std::span<const uint8_t> data, uint8_t buffer_slot)
{
  std::array<uint8_t, 1> const cmd_data = {buffer_slot};
  auto                         result  = co_await send_command_with_data(operation_code::download_model, cmd_data);

  if (!result.has_value())
  {
    co_return result.error();
  }

  // Delegate to protocol_handler for multi-packet data transfer
  size_t packet_size = m_cached_params ? m_cached_params->length : 128;
  auto send_result = co_await m_protocol.send_data_packets(data, packet_size);
  if (!send_result)
  {
    co_return send_result.error();
  }

  co_return status_code::success;
}

internal::task<std::optional<std::vector<uint8_t>>> fingerprint_sensor::download_image()
{
  auto result = co_await send_command_with_data(operation_code::upload_image, {});

  if (!result.has_value())
  {
    co_return std::nullopt;
  }

  // Delegate to protocol_handler for multi-packet data transfer
  auto data_result = co_await m_protocol.receive_data_packets(std::chrono::milliseconds{default_timeout_ms});
  if (!data_result.has_value())
  {
    co_return std::nullopt;
  }

  co_return data_result.value();
}

internal::task<status_code> fingerprint_sensor::upload_image(std::span<const uint8_t> data)
{
  auto result = co_await send_command_with_data(operation_code::download_image, {});

  if (!result.has_value())
  {
    co_return result.error();
  }

  // Delegate to protocol_handler for multi-packet data transfer
  size_t packet_size = m_cached_params ? m_cached_params->length : 128;
  auto send_result = co_await m_protocol.send_data_packets(data, packet_size);
  if (!send_result)
  {
    co_return send_result.error();
  }

  co_return status_code::success;
}

} // namespace carbio
