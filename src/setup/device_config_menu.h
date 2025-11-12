#pragma once

namespace carbio
{
class fingerprint_sensor;
}

void device_config_menu(carbio::fingerprint_sensor& s) noexcept;
void baud_rate_menu(carbio::fingerprint_sensor& s) noexcept;
void security_level_menu(carbio::fingerprint_sensor& s) noexcept;
void packet_length_menu(carbio::fingerprint_sensor& s) noexcept;
void device_password_menu(carbio::fingerprint_sensor& s) noexcept;
void current_settings_menu(carbio::fingerprint_sensor& s) noexcept;
void soft_reset_menu(carbio::fingerprint_sensor& s) noexcept;
