#ifndef CARBIO_FINGERPRINT_SENSOR_H
#define CARBIO_FINGERPRINT_SENSOR_H

#include "carbio/internal/fingerprint/baud_rate_setting.h"
#include "carbio/internal/fingerprint/device_setting_index.h"
#include "carbio/internal/fingerprint/device_setting_info.h"
#include "carbio/internal/fingerprint/led_color_setting.h"
#include "carbio/internal/fingerprint/led_mode_setting.h"
#include "carbio/internal/fingerprint/match_query_result.h"
#include "carbio/internal/fingerprint/operation_code.h"
#include "carbio/internal/fingerprint/packet_length_setting.h"
#include "carbio/internal/fingerprint/packet_type.h"
#include "carbio/internal/fingerprint/result.h"
#include "carbio/internal/fingerprint/search_query_info.h"
#include "carbio/internal/fingerprint/security_level_setting.h"
#include "carbio/internal/fingerprint/status_code.h"
#include "carbio/internal/fingerprint/protocol_handler.h"
#include "carbio/internal/io/event_loop.h"
#include "carbio/internal/io/serial_port.h"
#include "carbio/internal/io/task.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace carbio
{

// Import types from internal namespace for public API
using internal::status_code;
using internal::packet_type;
using internal::operation_code;
using internal::led_mode_setting;
using internal::led_color_setting;
using internal::baud_rate_setting;
using internal::security_level_setting;
using internal::packet_length_setting;
using internal::device_setting_index;
using internal::device_setting_info;
using internal::search_query_info;
using internal::match_query_info;
using internal::task;
using internal::result;
using internal::void_result;

//template <typename T>
//using result = internal::result<T>;
//using void_result = internal::void_result;

class fingerprint_sensor
{
public:
  static constexpr uint16_t start_code         = 0xEF01;
  static constexpr uint32_t default_address    = 0xFFFFFFFF;
  static constexpr int32_t  default_timeout_ms = 1000;
  static constexpr size_t   max_packet_size    = 256;
  static constexpr size_t   packet_header_size = 9;

  explicit fingerprint_sensor(internal::event_loop &loop, std::string_view device_path);
  ~fingerprint_sensor();

  fingerprint_sensor(fingerprint_sensor const &)            = delete;
  fingerprint_sensor &operator=(fingerprint_sensor const &) = delete;

  fingerprint_sensor(fingerprint_sensor &&) noexcept;
  fingerprint_sensor &operator=(fingerprint_sensor &&) noexcept;

  // ========================================
  // PUBLIC API - High-level workflows
  // ========================================

  // Connection management
  [[nodiscard]] internal::task<result<void>> connect(std::optional<int32_t> baud_rate = {});
  void disconnect();
  [[nodiscard]] bool is_connected() const;

  // High-level workflows
  [[nodiscard]] internal::task<result<void>> enroll(uint16_t template_id);
  [[nodiscard]] internal::task<result<search_query_info>> verify();
  [[nodiscard]] internal::task<result<search_query_info>> identify();

  // Template management
  [[nodiscard]] internal::task<result<std::vector<uint16_t>>> list_templates();
  [[nodiscard]] internal::task<result<uint16_t>> count_templates();
  [[nodiscard]] internal::task<result<void>> delete_template(uint16_t template_id);
  [[nodiscard]] internal::task<result<void>> clear_all();

  // Device information
  [[nodiscard]] internal::task<result<device_setting_info>> get_device_info();

  // Visual feedback
  [[nodiscard]] internal::task<result<void>> led_on();
  [[nodiscard]] internal::task<result<void>> led_off();

private:
  // ========================================
  // PRIVATE - Low-level sensor operations
  // ========================================

  // Connection helpers
  [[nodiscard]] internal::task<result<void>> try_connect_with_baud(int32_t baud_rate);

  // Low-level sensor commands (moved from public)
  [[nodiscard]] internal::task<status_code> verify_password();
  [[nodiscard]] internal::task<status_code> capture_image();
  [[nodiscard]] internal::task<status_code> image_to_template(uint8_t buffer_slot);
  [[nodiscard]] internal::task<status_code> create_model();
  [[nodiscard]] internal::task<status_code> store_template(uint16_t template_id, uint8_t buffer_slot);
  [[nodiscard]] internal::task<std::optional<search_query_info>> search(uint8_t buffer_slot, uint16_t start_id, uint16_t count);
  [[nodiscard]] internal::task<std::optional<search_query_info>> fast_search();
  [[nodiscard]] internal::task<status_code> load_template(uint16_t template_id, uint8_t buffer_slot);
  [[nodiscard]] internal::task<std::optional<std::vector<uint8_t>>> download_template(uint8_t buffer_slot);
  [[nodiscard]] internal::task<status_code> upload_template(std::span<const uint8_t> data, uint8_t buffer_slot);
  [[nodiscard]] internal::task<std::optional<std::vector<uint8_t>>> download_image();
  [[nodiscard]] internal::task<status_code> upload_image(std::span<const uint8_t> data);
  [[nodiscard]] internal::task<status_code> control_led(led_mode_setting mode, uint8_t speed, led_color_setting color, uint8_t cycles);
  [[nodiscard]] internal::task<status_code> turn_led_on();
  [[nodiscard]] internal::task<status_code> turn_led_off();
  [[nodiscard]] internal::task<std::optional<device_setting_info>> get_parameters();
  [[nodiscard]] internal::task<std::optional<uint16_t>> get_template_count();
  [[nodiscard]] internal::task<std::optional<std::vector<uint16_t>>> fetch_templates();
  [[nodiscard]] internal::task<status_code> clear_database();
  [[nodiscard]] internal::task<status_code> set_baud_rate(baud_rate_setting baud);
  [[nodiscard]] internal::task<status_code> set_security_level_register(security_level_setting level);
  [[nodiscard]] internal::task<status_code> set_data_packet_size_register(packet_length_setting packet_size);
  [[nodiscard]] internal::task<status_code> soft_reset();
  [[nodiscard]] internal::task<status_code> delete_template(uint16_t start_id, uint16_t count);

  // Protocol layer (delegates to protocol_handler)
  [[nodiscard]] internal::task<result<std::vector<uint8_t>>> send_command_with_data(operation_code cmd, std::span<const uint8_t> data = {});
  [[nodiscard]] internal::task<status_code> send_command(operation_code cmd, std::span<const uint8_t> data = {});

  // ========================================
  // MEMBERS
  // ========================================

  internal::event_loop& m_loop;
  internal::serial_port m_serial;
  internal::protocol_handler m_protocol;
  std::string m_port_name;
  std::uint32_t m_password;
  std::uint32_t m_address;
  std::optional<device_setting_info> m_cached_params;
};

} // namespace carbio

#endif // CARBIO_FINGERPRINT_SENSOR_H
