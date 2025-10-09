#include "carbio/fingerprint.h"
#include "carbio/internal/io/event_loop.h"

namespace carbio
{

// ========================================
// HIGH-LEVEL WORKFLOW IMPLEMENTATIONS
// ========================================

internal::task<result<void>> fingerprint_sensor::connect(std::optional<int32_t> baud_rate)
{
  if (baud_rate) {
    co_return co_await try_connect_with_baud(*baud_rate);
  }

  // Auto-detect baud rate
  constexpr std::array<int32_t, 12> baud_rates = {
    57600, 115200, 9600, 19200, 28800, 38400,
    48000, 67200, 76800, 86400, 96000, 105600
  };

  for (int32_t try_baud : baud_rates) {
    auto result = co_await try_connect_with_baud(try_baud);
    if (result) {
      co_return internal::make_success();
    }
  }

  co_return internal::make_error(status_code::communication_error);
}

internal::task<result<void>> fingerprint_sensor::try_connect_with_baud(int32_t baud_rate)
{
  internal::serial_config config;
  config.baud_rate = baud_rate;
  config.data_bits_val = internal::data_bits::eight;
  config.parity_val = internal::parity::none;
  config.stop_bits_val = internal::stop_bits::one;
  config.hardware_flow_control = false;
  config.low_latency = true;

  if (!m_serial.open(m_port_name, config)) {
    co_return internal::make_error(status_code::communication_error);
  }

  // Wait for sensor to stabilize
  co_await internal::sleep_awaitable{m_loop, std::chrono::milliseconds{100}};

  // Verify password
  auto status = co_await verify_password();
  if (status != status_code::success) {
    m_serial.close();
    co_return internal::make_error(status);
  }

  // Get system parameters
  auto params = co_await get_parameters();
  if (!params) {
    m_serial.close();
    co_return internal::make_error(status_code::communication_error);
  }

  m_cached_params = *params;
  m_protocol.set_device_address(m_address);

  co_return internal::make_success();
}

void fingerprint_sensor::disconnect()
{
  m_serial.close();
  m_cached_params.reset();
}

bool fingerprint_sensor::is_connected() const
{
  return m_serial.is_open();
}

internal::task<result<void>> fingerprint_sensor::enroll(uint16_t template_id)
{
  // Step 1: Capture first fingerprint
  auto r1 = co_await capture_image();
  if (r1 != status_code::success) {
    co_return internal::make_error(r1);
  }

  auto r2 = co_await image_to_template(1);
  if (r2 != status_code::success) {
    co_return internal::make_error(r2);
  }

  // Wait for user to lift finger and place again
  co_await internal::sleep_awaitable{m_loop, std::chrono::milliseconds{500}};

  // Step 2: Capture second fingerprint
  auto r3 = co_await capture_image();
  if (r3 != status_code::success) {
    co_return internal::make_error(r3);
  }

  auto r4 = co_await image_to_template(2);
  if (r4 != status_code::success) {
    co_return internal::make_error(r4);
  }

  // Step 3: Create model from two templates
  auto r5 = co_await create_model();
  if (r5 != status_code::success) {
    co_return internal::make_error(r5);
  }

  // Step 4: Store to flash
  auto r6 = co_await store_template(template_id, 1);
  if (r6 != status_code::success) {
    co_return internal::make_error(r6);
  }

  co_return internal::make_success();
}

internal::task<result<search_query_info>> fingerprint_sensor::verify()
{
  // Capture and convert
  auto r1 = co_await capture_image();
  if (r1 != status_code::success) {
    co_return internal::make_error<search_query_info>(r1);
  }

  auto r2 = co_await image_to_template(1);
  if (r2 != status_code::success) {
    co_return internal::make_error<search_query_info>(r2);
  }

  // Search in database
  auto result = co_await search(1, 0, 0);  // Search all
  if (!result) {
    co_return internal::make_error<search_query_info>(status_code::not_found);
  }

  co_return *result;
}

internal::task<result<search_query_info>> fingerprint_sensor::identify()
{
  // Capture and convert
  auto r1 = co_await capture_image();
  if (r1 != status_code::success) {
    co_return internal::make_error<search_query_info>(r1);
  }

  auto r2 = co_await image_to_template(1);
  if (r2 != status_code::success) {
    co_return internal::make_error<search_query_info>(r2);
  }

  // Fast search
  auto result = co_await fast_search();
  if (!result) {
    co_return internal::make_error<search_query_info>(status_code::not_found);
  }

  co_return *result;
}

internal::task<result<std::vector<uint16_t>>> fingerprint_sensor::list_templates()
{
  auto result = co_await fetch_templates();
  if (!result) {
    co_return internal::make_error<std::vector<uint16_t>>(status_code::communication_error);
  }
  co_return *result;
}

internal::task<result<uint16_t>> fingerprint_sensor::count_templates()
{
  auto result = co_await get_template_count();
  if (!result) {
    co_return internal::make_error<uint16_t>(status_code::communication_error);
  }
  co_return *result;
}

internal::task<result<void>> fingerprint_sensor::delete_template(uint16_t template_id)
{
  // Call the private low-level method (note: name collision - need to be careful)
  std::array<uint8_t, 4> const data = {
    static_cast<uint8_t>((template_id >> 8) & 0xFF),
    static_cast<uint8_t>(template_id & 0xFF),
    static_cast<uint8_t>(0 >> 8),  // count high byte
    static_cast<uint8_t>(1 & 0xFF)  // count = 1
  };
  auto status = co_await send_command(operation_code::erase_model, data);
  if (status != status_code::success) {
    co_return internal::make_error(status);
  }
  co_return internal::make_success();
}

internal::task<result<void>> fingerprint_sensor::clear_all()
{
  auto status = co_await clear_database();
  if (status != status_code::success) {
    co_return internal::make_error(status);
  }
  co_return internal::make_success();
}

internal::task<result<device_setting_info>> fingerprint_sensor::get_device_info()
{
  // Return cached if available
  if (m_cached_params) {
    co_return *m_cached_params;
  }

  // Otherwise fetch
  auto result = co_await get_parameters();
  if (!result) {
    co_return internal::make_error<device_setting_info>(status_code::communication_error);
  }

  m_cached_params = *result;
  co_return *result;
}

internal::task<result<void>> fingerprint_sensor::led_on()
{
  auto status = co_await turn_led_on();
  if (status != status_code::success) {
    co_return internal::make_error(status);
  }
  co_return internal::make_success();
}

internal::task<result<void>> fingerprint_sensor::led_off()
{
  auto status = co_await turn_led_off();
  if (status != status_code::success) {
    co_return internal::make_error(status);
  }
  co_return internal::make_success();
}

} // namespace carbio
