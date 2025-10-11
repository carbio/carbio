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

#ifndef CARBIO_PROFILE_STORE_H
#define CARBIO_PROFILE_STORE_H

#include "carbio/locked_buffer.h"
#include "carbio/result.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace carbio
{
/*!
 * @brief Driver profile information.
 *
 * Associates a fingerprint sensor template ID with driver identity
 * and access privileges. Used for authentication and audit logging.
 */
struct driver_profile
{
  std::uint16_t                                        template_id; /*!< fingerprint template id (0-127) */
  std::string                                          name;        /*!< driver name */
  bool                                                 is_admin;    /*!< admin privileges flag */
  std::chrono::time_point<std::chrono::system_clock> created_at;  /*!< creation timestamp */
  std::chrono::time_point<std::chrono::system_clock> modified_at; /*!< last modification timestamp */
};

/*!
 * @brief Secure profile storage with hardware-bound encryption.
 *
 * Manages driver profiles with persistent storage and hardware-bound
 * encryption. Profiles associate fingerprint template IDs with driver
 * information for authentication and access control.
 *
 * Security features:
 * - AES-256-GCM authenticated encryption
 * - Hardware-bound encryption key (CPU serial + key file)
 * - PBKDF2 key derivation (100,000 iterations)
 * - Encrypted storage prevents profile tampering
 */
class profile_store
{
  std::vector<driver_profile> profiles_;
  std::string                 storage_path_;

  static constexpr std::uint16_t MIN_TEMPLATE_ID = 0;
  static constexpr std::uint16_t MAX_TEMPLATE_ID = 127;
  static constexpr int           PBKDF2_ITERATIONS = 100000;
  static constexpr int           AES_KEY_SIZE_BYTES = 32; // AES-256

public:
  // --- construction / destruction / assignment

  profile_store() noexcept;
  explicit profile_store(std::string_view storage_path) noexcept;
  ~profile_store();
  profile_store(profile_store &&other) noexcept;
  profile_store &operator=(profile_store &&other) noexcept;
  profile_store(profile_store const &)            = delete;
  profile_store &operator=(profile_store const &) = delete;

  // --- profile management

  /*!
   * @brief Load profiles from encrypted storage.
   * @returns Success if profiles loaded, error code otherwise.
   */
  [[nodiscard]] void_result load_profiles() noexcept;

  /*!
   * @brief Save profiles to encrypted storage.
   * @returns Success if profiles saved, error code otherwise.
   */
  [[nodiscard]] void_result save_profiles() noexcept;

  /*!
   * @brief Add a new profile with automatic ID assignment.
   * @param name Driver name (e.g., "Sarah", "Peter").
   * @param is_admin Admin privileges flag.
   * @returns Assigned template ID on success, error code otherwise.
   */
  [[nodiscard]] result<std::uint16_t> add_profile(std::string_view name, bool is_admin) noexcept;

  /*!
   * @brief Add a new profile with specific template ID.
   * @param name Driver name.
   * @param template_id Fingerprint template ID (0-127).
   * @param is_admin Admin privileges flag.
   * @returns Success if profile added, error code otherwise.
   */
  [[nodiscard]] void_result add_profile_with_id(std::string_view name, std::uint16_t template_id,
                                                 bool is_admin) noexcept;

  /*!
   * @brief Delete a profile by template ID.
   * @param template_id Template ID to delete.
   * @returns Success if profile deleted, error code otherwise.
   */
  [[nodiscard]] void_result delete_profile(std::uint16_t template_id) noexcept;

  /*!
   * @brief Update an existing profile.
   * @param template_id Template ID to update.
   * @param new_name New driver name.
   * @param is_admin New admin privileges flag.
   * @returns Success if profile updated, error code otherwise.
   */
  [[nodiscard]] void_result update_profile(std::uint16_t template_id, std::string_view new_name,
                                            bool is_admin) noexcept;

  // --- profile queries

  /*!
   * @brief Get profile by template ID.
   * @param template_id Template ID to query.
   * @returns Profile if found, std::nullopt otherwise.
   */
  [[nodiscard]] std::optional<driver_profile> get_profile(std::uint16_t template_id) const noexcept;

  /*!
   * @brief Get driver name by template ID.
   * @param template_id Template ID to query.
   * @returns Driver name if found, "Unknown" otherwise.
   */
  [[nodiscard]] std::string get_driver_name(std::uint16_t template_id) const noexcept;

  /*!
   * @brief Check if template ID has admin privileges.
   * @param template_id Template ID to check.
   * @returns true if profile exists and has admin flag, false otherwise.
   */
  [[nodiscard]] bool is_admin_id(std::uint16_t template_id) const noexcept;

  /*!
   * @brief Get next available template ID.
   * @returns First available template ID (0-127), or std::nullopt if all used.
   */
  [[nodiscard]] std::optional<std::uint16_t> get_next_available_id() const noexcept;

  /*!
   * @brief Get all stored profiles.
   * @returns Vector of all profiles.
   */
  [[nodiscard]] std::vector<driver_profile> get_all_profiles() const noexcept;

  /*!
   * @brief Get number of stored profiles.
   * @returns Profile count.
   */
  [[nodiscard]] std::size_t get_profile_count() const noexcept;

  // --- validation

  /*!
   * @brief Check if template ID is valid.
   * @param template_id Template ID to validate.
   * @returns true if ID is in range [0, 127], false otherwise.
   */
  [[nodiscard]] static bool is_valid_template_id(std::uint16_t template_id) noexcept;

  /*!
   * @brief Check if profile exists for template ID.
   * @param template_id Template ID to check.
   * @returns true if profile exists, false otherwise.
   */
  [[nodiscard]] bool profile_exists(std::uint16_t template_id) const noexcept;

  /*!
   * @brief Clear all profiles from memory (does not affect storage).
   */
  void clear_profiles() noexcept;

  /*!
   * @brief Clear all profiles from memory and storage.
   * @returns Success if profiles cleared and saved, error code otherwise.
   */
  [[nodiscard]] void_result clear_all_profiles() noexcept;

private:
  // encryption
  [[nodiscard]] locked_buffer<std::uint8_t> derive_encryption_key() const noexcept;
  [[nodiscard]] std::vector<std::uint8_t> encrypt_data(const std::vector<std::uint8_t> &plaintext,
                                                        const locked_buffer<std::uint8_t> &key) const noexcept;
  [[nodiscard]] std::vector<std::uint8_t> decrypt_data(const std::vector<std::uint8_t> &ciphertext,
                                                        const locked_buffer<std::uint8_t> &key) const noexcept;

  // hardware binding
  [[nodiscard]] std::vector<std::uint8_t> read_cpu_serial() const noexcept;
  [[nodiscard]] std::vector<std::uint8_t> read_encryption_key_file() const noexcept;

  // serialization
  [[nodiscard]] std::vector<std::uint8_t> serialize_profiles() const noexcept;
  [[nodiscard]] bool deserialize_profiles(const std::vector<std::uint8_t> &data) noexcept;

  // security
  static void secure_clear_memory(std::vector<std::uint8_t> &data) noexcept;
};

/*!
 * @brief Convert driver profile to JSON format string.
 * @param profile The driver profile.
 * @returns JSON representation of the profile.
 */
extern std::string to_json(const driver_profile &profile) noexcept;

} // namespace carbio

#endif // CARBIO_PROFILE_STORE_H
