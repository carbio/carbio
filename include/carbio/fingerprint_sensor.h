#ifndef CARBIO_FINGERPRINT_SENSOR_H
#define CARBIO_FINGERPRINT_SENSOR_H

#include "carbio/baud_rate_setting.h"
#include "carbio/command_code.h"
#include "carbio/command_executor.h"
#include "carbio/data_length_setting.h"
#include "carbio/device_info.h"
#include "carbio/device_setting_index.h"
#include "carbio/device_setting_info.h"
#include "carbio/led_color_setting.h"
#include "carbio/led_mode_setting.h"
#include "carbio/match_query_info.h"
#include "carbio/protocol_handler.h"
#include "carbio/result.h"
#include "carbio/search_query_info.h"
#include "carbio/security_level_setting.h"
#include "carbio/serial_port.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace carbio
{
class fingerprint_sensor
{
  serial_port      serial_;
  protocol_handler protocol_;
  command_executor executor_;

public:
  // --- construction / destruction / assignment

  fingerprint_sensor();
  ~fingerprint_sensor();
  fingerprint_sensor(fingerprint_sensor &&other) noexcept;
  fingerprint_sensor &operator=(fingerprint_sensor &&other) noexcept;
  fingerprint_sensor(fingerprint_sensor const &)            = delete;
  fingerprint_sensor &operator=(fingerprint_sensor const &) = delete;

  // --- connection management
  [[nodiscard]] bool is_open() const noexcept;
  [[nodiscard]] bool open(std::string_view path) noexcept;
  void               close() noexcept;

  // --- device configuration

  [[nodiscard]] result<device_setting_info> get_device_setting_info() noexcept;
  [[nodiscard]] void_result                 set_baud_rate_setting(baud_rate_setting setting) noexcept;
  [[nodiscard]] void_result                 set_security_level_setting(security_level_setting setting) noexcept;
  [[nodiscard]] void_result                 set_data_length_setting(data_length_setting setting) noexcept;
  [[nodiscard]] void_result                 soft_reset_device() noexcept;

  // --- led management
  [[nodiscard]] void_result set_led_setting(carbio::led_mode_setting mode, uint8_t speed = 128,
                                            carbio::led_color_setting color  = led_color_setting::blue,
                                            uint8_t           cycles = 0) noexcept;
  [[nodiscard]] void_result turn_led_on() noexcept;
  [[nodiscard]] void_result turn_led_off() noexcept;

  // --- security
  [[nodiscard]] void_result set_device_password(std::uint32_t password = 0x00000000) noexcept;
  [[nodiscard]] void_result verify_device_password(std::uint32_t password = 0x00000000) noexcept;
  [[nodiscard]] void_result set_device_address(std::uint32_t new_address) noexcept;

  // --- low-level ops
  [[nodiscard]] void_result capture_image() noexcept;
  [[nodiscard]] void_result extract_features(std::uint8_t buffer_id = 1);
  [[nodiscard]] void_result create_model() noexcept;
  [[nodiscard]] void_result store_model(std::uint16_t page_id, std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] void_result load_model(std::uint16_t page_id, std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] void_result upload_model(std::span<const std::uint8_t> buffe, std::uint8_t buffer_id = 1) noexcept;
  [[nodiscard]] void_result download_model(std::span<std::uint8_t> buffer) noexcept;

  // --- database ops
  [[nodiscard]] void_result           upload_image(std::span<const std::uint8_t> data) noexcept;
  [[nodiscard]] void_result           download_image(std::span<std::uint8_t> data) noexcept;
  [[nodiscard]] void_result           erase_model(std::uint16_t page_id, std::uint16_t count = 1) noexcept;
  [[nodiscard]] void_result           clear_database() noexcept;
  [[nodiscard]] result<match_query_info> match_model() noexcept;
  [[nodiscard]] result<search_query_info> search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1,
                                                   std::uint16_t count = 0) noexcept;
  [[nodiscard]] result<search_query_info> fast_search_model(std::uint16_t page_id = 0, std::uint8_t buffer_id = 1,
                                                        std::uint16_t count = 0) noexcept;
  [[nodiscard]] result<std::uint16_t> model_count() noexcept;
  [[nodiscard]] result<std::vector<std::uint8_t>> read_index_table(std::span<std::uint8_t> data) noexcept;
};
} /* namespace carbio */

#endif // CARBIO_FINGERPRINT_SENSOR_H
