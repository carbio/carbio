/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#ifndef CARBIO_STATUS_CODE_H
#define CARBIO_STATUS_CODE_H

#include <cstdint>
#include <string_view>

namespace carbio
{
/*!
 * @brief The status code of a operation.
 */
enum class status_code : std::uint8_t
{
  success                       = 0x00, /*!< Operation succeeded. */
  frame_error                   = 0x01, /*!< Frame error. */
  no_finger                     = 0x02, /*!< No finger detected. */
  image_capture_error           = 0x03, /*!< Fingerprint capture failed. */
  image_too_faint               = 0x04, /*!< Image is too faint. */
  image_too_blurry              = 0x05, /*!< Image is too blurry. */
  image_too_distorted           = 0x06, /*!< Image is too distorted. */
  image_too_few_features        = 0x07, /*!< Image has too few feature points. */
  no_match                      = 0x08, /*!< No match. */
  not_found                     = 0x09, /*!< Not found. */
  enrollment_mismatch           = 0x0A, /*!< Enrollment mismatch. */
  index_out_of_range            = 0x0B, /*!< Index out of range. */
  database_access_error         = 0x0C, /*!< Database access failed. */
  feature_upload_failed         = 0x0D, /*!< Feature upload failed.  */
  no_frame                      = 0x0E, /*!< No frame. */
  image_upload_failed           = 0x0F, /*!< Image upload failed. */
  erase_failed                  = 0x10, /*!< Erase failed. */
  database_clear_failed         = 0x11, /*!< Database clear failed. */
  cannot_enter_low_power_mode   = 0x12, /*!< Cannot enter low-power mode. */
  permission_denied             = 0x13, /*!< Permission denied. */
  invalid_image_format          = 0x15, /*!< Invalid or corrupt image format. */
  flash_write_error             = 0x18, /*!< Flash write error. */
  unknown_error                 = 0x19, /*!< Unknown error. */
  illegal_device_register       = 0x1A, /*!< Illegal access to invalid or non-existent device register. */
  invalid_device_configuration  = 0x1B, /*!< Invalid or unsupported device configuration. */
  communication_error           = 0x1D, /*!< Communication channel is inaccessible. */
  database_full                 = 0x1F, /*!< Database is full. */
  illegal_device_address        = 0x20, /*!< Illegal access to invalid or non-existent device address. */
  device_authorization_required = 0x21, /*!< Device requires password authorization prior proper use. */
  hardware_fault                = 0x29, /*!< Operation failed due hardware failure. */
  bad_packet                    = 0xFE, /*!< Invalid or illegal packet sent. */
  timeout                       = 0xFF, /*!< Operation timed out. */
};

/*!
 * @brief Converts the \c status to underlying string representation.
 * @param status The status value.
 * @returns The underlying string representation of \c status </c>.
 */
[[nodiscard]] inline constexpr std::string_view get_name(status_code status) noexcept
{
  switch (status)
  {
    case status_code::success:
      return "success";
    case status_code::frame_error:
      return "frame_error";
    case status_code::no_finger:
      return "no_finger";
    case status_code::image_capture_error:
      return "image_capture_error";
    case status_code::image_too_faint:
      return "image_too_faint";
    case status_code::image_too_blurry:
      return "image_too_blurry";
    case status_code::image_too_distorted:
      return "image_too_distorted";
    case status_code::image_too_few_features:
      return "image_too_few_features";
    case status_code::no_match:
      return "no_match";
    case status_code::not_found:
      return "not_found";
    case status_code::enrollment_mismatch:
      return "enrollment_mismatch";
    case status_code::index_out_of_range:
      return "index_out_of_range";
    case status_code::database_access_error:
      return "database_access_error";
    case status_code::feature_upload_failed:
      return "feature_upload_failed";
    case status_code::no_frame:
      return "no_frame";
    case status_code::image_upload_failed:
      return "image_upload_failed";
    case status_code::erase_failed:
      return "erase_failed";
    case status_code::database_clear_failed:
      return "database_clear_failed";
    case status_code::cannot_enter_low_power_mode:
      return "cannot_enter_low_power_mode";
    case status_code::permission_denied:
      return "permission_denied";
    case status_code::invalid_image_format:
      return "invalid_image_format";
    case status_code::flash_write_error:
      return "flash_write_error";
    case status_code::unknown_error:
      return "unknown_error";
    case status_code::illegal_device_register:
      return "illegal_device_register";
    case status_code::invalid_device_configuration:
      return "invalid_device_configuration";
    case status_code::communication_error:
      return "communication_error";
    case status_code::database_full:
      return "database_full";
    case status_code::illegal_device_address:
      return "illegal_device_address";
    case status_code::device_authorization_required:
      return "device_authorization_required";
    case status_code::hardware_fault:
      return "hardware_fault";
    case status_code::bad_packet:
      return "bad_packet";
    case status_code::timeout:
      return "timeout";
  }
};

/*!
 * @brief Converts the \c status to hexadecimal representation as string.
 * @param value The status value.
 * @returns The string representation of \c setting </c> in hexadecimal format.
 */
[[nodiscard]] inline constexpr std::string_view get_hex_str(status_code status) noexcept
{
  switch (status)
  {
    case status_code::success:
      return "0x00";
    case status_code::frame_error:
      return "0x01";
    case status_code::no_finger:
      return "0x02";
    case status_code::image_capture_error:
      return "0x03";
    case status_code::image_too_faint:
      return "0x04";
    case status_code::image_too_blurry:
      return "0x05";
    case status_code::image_too_distorted:
      return "0x06";
    case status_code::image_too_few_features:
      return "0x07";
    case status_code::no_match:
      return "0x08";
    case status_code::not_found:
      return "0x09";
    case status_code::enrollment_mismatch:
      return "0x0A";
    case status_code::index_out_of_range:
      return "0x0B";
    case status_code::database_access_error:
      return "0x0C";
    case status_code::feature_upload_failed:
      return "0x0D";
    case status_code::no_frame:
      return "0x0E";
    case status_code::image_upload_failed:
      return "0x0F";
    case status_code::erase_failed:
      return "0x10";
    case status_code::database_clear_failed:
      return "0x11";
    case status_code::cannot_enter_low_power_mode:
      return "0x12";
    case status_code::permission_denied:
      return "0x13";
    case status_code::invalid_image_format:
      return "0x15";
    case status_code::flash_write_error:
      return "0x18";
    case status_code::unknown_error:
      return "0x19";
    case status_code::illegal_device_register:
      return "0x1A";
    case status_code::invalid_device_configuration:
      return "0x1B";
    case status_code::communication_error:
      return "0x1D";
    case status_code::database_full:
      return "0x1F";
    case status_code::illegal_device_address:
      return "0x20";
    case status_code::device_authorization_required:
      return "0x21";
    case status_code::hardware_fault:
      return "0x29";
    case status_code::bad_packet:
      return "0xFE";
    case status_code::timeout:
      return "0xFF";
  }
};

/*!
 * @brief Convert the \c status to human readable message.
 * @param status The status value.
 * @returns A human readable string representation of \c status </c>.
 */
[[nodiscard]] inline constexpr std::string_view get_message(status_code status) noexcept
{
  switch (status)
  {
    case status_code::success:
      return "operation succeeded";
    case status_code::frame_error:
      return "packet frame receive error";
    case status_code::no_finger:
      return "no finger detected";
    case status_code::image_capture_error:
      return "image capture error";
    case status_code::image_too_faint:
      return "image too faint";
    case status_code::image_too_blurry:
      return "image too blurry";
    case status_code::image_too_distorted:
      return "image too distorted";
    case status_code::image_too_few_features:
      return "image contains too few feature points";
    case status_code::no_match:
      return "fingerprint not match";
    case status_code::not_found:
      return "fingerprint not found";
    case status_code::enrollment_mismatch:
      return "enrollment mismatch";
    case status_code::index_out_of_range:
      return "index is out of range";
    case status_code::database_access_error:
      return "storage access error";
    case status_code::feature_upload_failed:
      return "feature upload failed";
    case status_code::no_frame:
      return "no packet frame";
    case status_code::image_upload_failed:
      return "image upload failed";
    case status_code::erase_failed:
      return "erase failed";
    case status_code::database_clear_failed:
      return "failed deleting database ";
    case status_code::cannot_enter_low_power_mode:
      return "cannot enter low power mode";
    case status_code::permission_denied:
      return "permission denied; incorrect device password";
    case status_code::invalid_image_format:
      return "invalid image format";
    case status_code::flash_write_error:
      return "flash write failed";
    case status_code::unknown_error:
      return "unknown error";
    case status_code::illegal_device_register:
      return "illegal access to non-existent or invalid device register";
    case status_code::invalid_device_configuration:
      return "invalid device configuration; configuration rejected";
    case status_code::communication_error:
      return "communication interface inaccessible";
    case status_code::database_full:
      return "database is full";
    case status_code::illegal_device_address:
      return "illegal access to non-existent or invalid device address";
    case status_code::device_authorization_required:
      return "device requires password authorization prior use";
    case status_code::hardware_fault:
      return "operation failed due hardware failure";
    case status_code::bad_packet:
      return "bad packet";
    case status_code::timeout:
      return "operation timed out";
  }
};

/*!
 * @brief Determines whether the status code indicates successful operation or not.
 * @param status The status code.
 * @returns \c true if </c> \c status </c> is \c success </c>, \c false otherwise.
 */
[[nodiscard]] inline constexpr bool is_success(status_code status) noexcept
{
  return status_code::success == status;
};

/*!
 * @brief Determines whether the status code indicates erroneous operation or not.
 * @param status The status code.
 * @returns \c true if </c> \c status </c> is not \c success </c>, \c false otherwise.
 */
[[nodiscard]] inline constexpr bool is_error(status_code status) noexcept
{
  return !is_success(status);
};
} // namespace carbio

#endif // CARBIO_STATUS_CODE_H
