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

#include "carbio/profile_store.h"
#include "carbio/locked_buffer.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(__cpp_lib_format)
#include <format>
#endif

// OpenSSL for encryption
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace carbio
{
namespace
{
// CPU serial path (Linux-specific)
constexpr std::string_view CPU_SERIAL_PATH = "/proc/cpuinfo";

/*!
 * @brief Get user's home directory path.
 */
[[nodiscard]] std::filesystem::path get_home_directory() noexcept
{
  try
  {
    // try HOME environment variable first (POSIX)
    if (const char *home = std::getenv("HOME"))
    {
      return std::filesystem::path(home);
    }

    // fallback to /home/username on Linux
#ifdef __linux__
    if (const char *user = std::getenv("USER"))
    {
      return std::filesystem::path("/home") / user;
    }
#endif

    // last resort - current path
    return std::filesystem::current_path();
  }
  catch (...)
  {
    return std::filesystem::path("/tmp");
  }
}

/*!
 * @brief Get default config directory path.
 */
[[nodiscard]] std::filesystem::path get_config_directory() noexcept
{
  try
  {
    // follow XDG Base Directory specification
    if (const char *xdg_config = std::getenv("XDG_CONFIG_HOME"))
    {
      return std::filesystem::path(xdg_config) / "carbiod";
    }

    // fallback to ~/.config/carbiod
    return get_home_directory() / ".config" / "carbiod";
  }
  catch (...)
  {
    return std::filesystem::path("/tmp/carbiod");
  }
}

/*!
 * @brief Get default storage file path.
 */
[[nodiscard]] std::string get_default_storage_path() noexcept
{
  return (get_config_directory() / "profiles.dat").string();
}

/*!
 * @brief Get encryption key file path.
 */
[[nodiscard]] std::string get_encryption_key_path() noexcept
{
  return (get_config_directory() / "encryption_key.dat").string();
}

// AES-GCM parameters
constexpr std::size_t GCM_IV_LENGTH  = 12; // 96 bits recommended for GCM
constexpr std::size_t GCM_TAG_LENGTH = 16; // 128 bits authentication tag

/*!
 * @brief Read file contents into byte vector.
 */
[[nodiscard]] std::vector<std::uint8_t> read_file_bytes(const std::filesystem::path &path) noexcept
{
  try
  {
    if (!std::filesystem::exists(path))
    {
      return {};
    }

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
      return {};
    }

    file.seekg(0, std::ios::end);
    const auto file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(file_size));
    file.read(reinterpret_cast<char *>(buffer.data()), file_size);

    return buffer;
  }
  catch (...)
  {
    return {};
  }
}

/*!
 * @brief Write byte vector to file.
 */
[[nodiscard]] bool write_file_bytes(const std::filesystem::path &path,
                                     const std::vector<std::uint8_t> &data) noexcept
{
  try
  {
    // ensure parent directory exists
    const auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent))
    {
      std::filesystem::create_directories(parent);
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
      return false;
    }

    file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    return file.good();
  }
  catch (...)
  {
    return false;
  }
}

/*!
 * @brief Derive key using PBKDF2-SHA256 with secure memory.
 */
[[nodiscard]] locked_buffer<std::uint8_t> pbkdf2_derive_key(const locked_buffer<std::uint8_t> &password,
                                                              const std::vector<std::uint8_t> &salt,
                                                              int                              iterations,
                                                              std::size_t key_length) noexcept
{
  locked_buffer<std::uint8_t> key(key_length);

  const int result = PKCS5_PBKDF2_HMAC(reinterpret_cast<const char *>(password.data()),
                                        static_cast<int>(password.size()), salt.data(),
                                        static_cast<int>(salt.size()), iterations, EVP_sha256(),
                                        static_cast<int>(key_length), key.data());

  if (result != 1)
  {
    return locked_buffer<std::uint8_t>{}; // empty buffer on failure
  }

  return key;
}

} // anonymous namespace

// --- construction / destruction / assignment

profile_store::profile_store() noexcept : storage_path_(get_default_storage_path())
{
}

profile_store::profile_store(std::string_view storage_path) noexcept : storage_path_(storage_path)
{
}

profile_store::~profile_store() = default;

profile_store::profile_store(profile_store &&other) noexcept
    : profiles_(std::move(other.profiles_)), storage_path_(std::move(other.storage_path_))
{
}

profile_store &profile_store::operator=(profile_store &&other) noexcept
{
  if (this != &other)
  {
    profiles_     = std::move(other.profiles_);
    storage_path_ = std::move(other.storage_path_);
  }
  return *this;
}

// --- profile management

void_result profile_store::load_profiles() noexcept
{
  const auto encrypted_data = read_file_bytes(storage_path_);
  if (encrypted_data.empty())
  {
    // empty file or doesn't exist - not an error, start fresh
    profiles_.clear();
    return make_success();
  }

  const auto key = derive_encryption_key();
  if (key.empty())
  {
    return make_error(status_code::database_access_error);
  }

  const auto plaintext = decrypt_data(encrypted_data, key);
  if (plaintext.empty())
  {
    return make_error(status_code::database_access_error);
  }

  if (!deserialize_profiles(plaintext))
  {
    return make_error(status_code::database_access_error);
  }

  return make_success();
}

void_result profile_store::save_profiles() noexcept
{
  const auto plaintext = serialize_profiles();
  if (plaintext.empty())
  {
    return make_error(status_code::database_access_error);
  }

  const auto key = derive_encryption_key();
  if (key.empty())
  {
    return make_error(status_code::database_access_error);
  }

  const auto ciphertext = encrypt_data(plaintext, key);
  if (ciphertext.empty())
  {
    return make_error(status_code::database_access_error);
  }

  if (!write_file_bytes(storage_path_, ciphertext))
  {
    return make_error(status_code::database_access_error);
  }

  return make_success();
}

result<std::uint16_t> profile_store::add_profile(std::string_view name, bool is_admin) noexcept
{
  if (name.empty() || name.length() > 64)
  {
    return make_error(status_code::permission_denied);
  }

  const auto next_id = get_next_available_id();
  if (!next_id.has_value())
  {
    return make_error(status_code::database_full);
  }

  const auto template_id = next_id.value();
  const auto result      = add_profile_with_id(name, template_id, is_admin);
  if (!result)
  {
    return make_error(result.error());
  }

  return make_success(template_id);
}

void_result profile_store::add_profile_with_id(std::string_view name, std::uint16_t template_id,
                                                bool is_admin) noexcept
{
  if (!is_valid_template_id(template_id))
  {
    return make_error(status_code::index_out_of_range);
  }

  if (name.empty() || name.length() > 64)
  {
    return make_error(status_code::permission_denied);
  }

  if (profile_exists(template_id))
  {
    return make_error(status_code::permission_denied);
  }

  driver_profile profile;
  profile.template_id = template_id;
  profile.name        = std::string(name);
  profile.is_admin    = is_admin;
  profile.created_at  = std::chrono::system_clock::now();
  profile.modified_at = profile.created_at;

  profiles_.push_back(std::move(profile));

  // save immediately
  return save_profiles();
}

void_result profile_store::delete_profile(std::uint16_t template_id) noexcept
{
  const auto it = std::ranges::find_if(profiles_, [template_id](const auto &p) noexcept { return p.template_id == template_id; });

  if (it == profiles_.end())
  {
    return make_error(status_code::not_found);
  }

  profiles_.erase(it);

  // save immediately
  return save_profiles();
}

void_result profile_store::update_profile(std::uint16_t template_id, std::string_view new_name, bool is_admin) noexcept
{
  if (new_name.empty() || new_name.length() > 64)
  {
    return make_error(status_code::permission_denied);
  }

  auto profile = std::ranges::find_if(profiles_, [template_id](const auto &p) noexcept { return p.template_id == template_id; });

  if (profile == profiles_.end())
  {
    return make_error(status_code::not_found);
  }

  profile->name        = std::string(new_name);
  profile->is_admin    = is_admin;
  profile->modified_at = std::chrono::system_clock::now();

  // save immediately
  return save_profiles();
}

// --- profile queries

std::optional<driver_profile> profile_store::get_profile(std::uint16_t template_id) const noexcept
{
  const auto it = std::ranges::find_if(profiles_, [template_id](const auto &p) noexcept { return p.template_id == template_id; });

  if (it != profiles_.end())
  {
    return *it;
  }

  return std::nullopt;
}

std::string profile_store::get_driver_name(std::uint16_t template_id) const noexcept
{
  const auto profile = get_profile(template_id);
  if (profile.has_value())
  {
    return profile->name;
  }
  return "Unknown";
}

bool profile_store::is_admin_id(std::uint16_t template_id) const noexcept
{
  const auto profile = get_profile(template_id);
  return profile.has_value() && profile->is_admin;
}

std::optional<std::uint16_t> profile_store::get_next_available_id() const noexcept
{
  for (std::uint16_t id = MIN_TEMPLATE_ID; id <= MAX_TEMPLATE_ID; ++id)
  {
    if (!profile_exists(id))
    {
      return id;
    }
  }
  return std::nullopt;
}

std::vector<driver_profile> profile_store::get_all_profiles() const noexcept
{
  return profiles_;
}

std::size_t profile_store::get_profile_count() const noexcept
{
  return profiles_.size();
}

// --- validation

bool profile_store::is_valid_template_id(std::uint16_t template_id) noexcept
{
  return template_id >= MIN_TEMPLATE_ID && template_id <= MAX_TEMPLATE_ID;
}

bool profile_store::profile_exists(std::uint16_t template_id) const noexcept
{
  return std::ranges::any_of(profiles_, [template_id](const auto &p) noexcept { return p.template_id == template_id; });
}

void profile_store::clear_profiles() noexcept
{
  profiles_.clear();
}

void_result profile_store::clear_all_profiles() noexcept
{
  profiles_.clear();
  return save_profiles();
}

// --- encryption (private)

locked_buffer<std::uint8_t> profile_store::derive_encryption_key() const noexcept
{
  // combine CPU serial + key file for hardware binding
  auto cpu_serial = read_cpu_serial();
  auto key_file   = read_encryption_key_file();

  if (cpu_serial.empty() && key_file.empty())
  {
    return locked_buffer<std::uint8_t>{};
  }

  // combine both sources in secure memory
  locked_buffer<std::uint8_t> password(cpu_serial.size() + key_file.size());
  std::copy(cpu_serial.begin(), cpu_serial.end(), password.begin());
  std::copy(key_file.begin(), key_file.end(), password.begin() + cpu_serial.size());

  // use a fixed salt (not ideal but necessary for deterministic key derivation)
  const std::string salt_str = "carbio_profile_store_v1";
  std::vector<std::uint8_t> salt(salt_str.begin(), salt_str.end());

  auto key = pbkdf2_derive_key(password, salt, PBKDF2_ITERATIONS, AES_KEY_SIZE_BYTES);

  // password is automatically securely cleared when it goes out of scope

  return key;
}

std::vector<std::uint8_t> profile_store::encrypt_data(const std::vector<std::uint8_t> &plaintext,
                                                       const locked_buffer<std::uint8_t> &key) const noexcept
{
  if (plaintext.empty() || key.size() != AES_KEY_SIZE_BYTES)
  {
    return {};
  }

  // generate random IV
  std::vector<std::uint8_t> iv(GCM_IV_LENGTH);
  if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1)
  {
    return {};
  }

  // create cipher context
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    return {};
  }

  // initialize encryption
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  // allocate output buffer
  std::vector<std::uint8_t> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
  int                       ciphertext_len = 0;

  // encrypt
  if (EVP_EncryptUpdate(ctx, ciphertext.data(), &ciphertext_len, plaintext.data(),
                        static_cast<int>(plaintext.size())) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  int final_len = 0;
  if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertext_len, &final_len) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  ciphertext_len += final_len;
  ciphertext.resize(ciphertext_len);

  // get authentication tag
  std::vector<std::uint8_t> tag(GCM_TAG_LENGTH);
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data()) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  EVP_CIPHER_CTX_free(ctx);

  // format: IV || ciphertext || tag
  std::vector<std::uint8_t> result;
  result.reserve(iv.size() + ciphertext.size() + tag.size());
  result.insert(result.end(), iv.begin(), iv.end());
  result.insert(result.end(), ciphertext.begin(), ciphertext.end());
  result.insert(result.end(), tag.begin(), tag.end());

  return result;
}

std::vector<std::uint8_t> profile_store::decrypt_data(const std::vector<std::uint8_t> &ciphertext,
                                                       const locked_buffer<std::uint8_t> &key) const noexcept
{
  if (ciphertext.size() < GCM_IV_LENGTH + GCM_TAG_LENGTH || key.size() != AES_KEY_SIZE_BYTES)
  {
    return {};
  }

  // extract components
  const auto *iv_ptr         = ciphertext.data();
  const auto *encrypted_data = ciphertext.data() + GCM_IV_LENGTH;
  const auto  encrypted_len  = ciphertext.size() - GCM_IV_LENGTH - GCM_TAG_LENGTH;
  const auto *tag_ptr        = ciphertext.data() + GCM_IV_LENGTH + encrypted_len;

  // create cipher context
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    return {};
  }

  // initialize decryption
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv_ptr) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  // allocate output buffer
  std::vector<std::uint8_t> plaintext(encrypted_len + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
  int                       plaintext_len = 0;

  // decrypt
  if (EVP_DecryptUpdate(ctx, plaintext.data(), &plaintext_len, encrypted_data, static_cast<int>(encrypted_len)) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  // set expected tag
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_LENGTH, const_cast<std::uint8_t *>(tag_ptr)) != 1)
  {
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  int final_len = 0;
  if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintext_len, &final_len) != 1)
  {
    // authentication failed
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  plaintext_len += final_len;
  plaintext.resize(plaintext_len);

  EVP_CIPHER_CTX_free(ctx);

  return plaintext;
}

// --- hardware binding (private)

std::vector<std::uint8_t> profile_store::read_cpu_serial() const noexcept
{
  const auto cpuinfo = read_file_bytes(CPU_SERIAL_PATH);
  if (cpuinfo.empty())
  {
    return {};
  }

  // extract serial from /proc/cpuinfo
  std::string cpuinfo_str(cpuinfo.begin(), cpuinfo.end());
  std::istringstream iss(cpuinfo_str);
  std::string        line;

  while (std::getline(iss, line))
  {
    if (line.starts_with("Serial"))
    {
      const auto colon_pos = line.find(':');
      if (colon_pos != std::string::npos)
      {
        auto serial = line.substr(colon_pos + 1);
        // trim whitespace
        serial.erase(0, serial.find_first_not_of(" \t"));
        serial.erase(serial.find_last_not_of(" \t\r\n") + 1);

        return std::vector<std::uint8_t>(serial.begin(), serial.end());
      }
    }
  }

  return {};
}

std::vector<std::uint8_t> profile_store::read_encryption_key_file() const noexcept
{
  return read_file_bytes(get_encryption_key_path());
}

// --- serialization (private)

std::vector<std::uint8_t> profile_store::serialize_profiles() const noexcept
{
  // simple JSON serialization
  std::ostringstream oss;
  oss << "{\n  \"version\": 1,\n  \"profiles\": [\n";

  for (std::size_t i = 0; i < profiles_.size(); ++i)
  {
    const auto &p = profiles_[i];

    const auto created_time =
        std::chrono::duration_cast<std::chrono::seconds>(p.created_at.time_since_epoch()).count();
    const auto modified_time =
        std::chrono::duration_cast<std::chrono::seconds>(p.modified_at.time_since_epoch()).count();

    oss << "    {\n";
    oss << "      \"template_id\": " << p.template_id << ",\n";
    oss << "      \"name\": \"" << p.name << "\",\n";
    oss << "      \"is_admin\": " << (p.is_admin ? "true" : "false") << ",\n";
    oss << "      \"created_at\": " << created_time << ",\n";
    oss << "      \"modified_at\": " << modified_time << "\n";
    oss << "    }";

    if (i < profiles_.size() - 1)
    {
      oss << ",";
    }
    oss << "\n";
  }

  oss << "  ]\n}\n";

  const auto json_str = oss.str();
  return std::vector<std::uint8_t>(json_str.begin(), json_str.end());
}

bool profile_store::deserialize_profiles(const std::vector<std::uint8_t> &data) noexcept
{
  // simple JSON parsing (manual, no external dependencies)
  const std::string json_str(data.begin(), data.end());

  profiles_.clear();

  // very basic parsing - look for profile objects
  std::size_t pos = 0;
  while ((pos = json_str.find("\"template_id\":", pos)) != std::string::npos)
  {
    driver_profile profile;

    // parse template_id
    pos += 14; // skip "template_id":
    const auto id_start = json_str.find_first_of("0123456789", pos);
    const auto id_end   = json_str.find_first_not_of("0123456789", id_start);
    profile.template_id = static_cast<std::uint16_t>(std::stoi(json_str.substr(id_start, id_end - id_start)));

    // parse name
    const auto name_start = json_str.find("\"name\": \"", pos) + 9;
    const auto name_end   = json_str.find("\"", name_start);
    profile.name          = json_str.substr(name_start, name_end - name_start);

    // parse is_admin
    const auto admin_pos = json_str.find("\"is_admin\":", pos);
    profile.is_admin     = json_str.find("true", admin_pos) < json_str.find("false", admin_pos);

    // parse timestamps
    const auto created_pos  = json_str.find("\"created_at\":", pos) + 13;
    const auto created_end  = json_str.find_first_not_of("0123456789", created_pos);
    const auto created_time = std::stoll(json_str.substr(created_pos, created_end - created_pos));
    profile.created_at      = std::chrono::system_clock::time_point(std::chrono::seconds(created_time));

    const auto modified_pos  = json_str.find("\"modified_at\":", pos) + 14;
    const auto modified_end  = json_str.find_first_not_of("0123456789", modified_pos);
    const auto modified_time = std::stoll(json_str.substr(modified_pos, modified_end - modified_pos));
    profile.modified_at      = std::chrono::system_clock::time_point(std::chrono::seconds(modified_time));

    profiles_.push_back(std::move(profile));

    pos = modified_end;
  }

  return true;
}

// --- security (private, static)

void profile_store::secure_clear_memory(std::vector<std::uint8_t> &data) noexcept
{
  if (!data.empty())
  {
    // use volatile to prevent optimization
    volatile std::uint8_t *ptr = data.data();
    for (std::size_t i = 0; i < data.size(); ++i)
    {
      ptr[i] = 0;
    }
  }
}

// --- JSON conversion

std::string to_json(const driver_profile &profile) noexcept
{
  const auto created_time =
      std::chrono::duration_cast<std::chrono::seconds>(profile.created_at.time_since_epoch()).count();
  const auto modified_time =
      std::chrono::duration_cast<std::chrono::seconds>(profile.modified_at.time_since_epoch()).count();

#if defined(__cpp_lib_format)
  return std::format("{{ \"template_id\": {}, \"name\": \"{}\", \"is_admin\": {}, \"created_at\": {}, "
                     "\"modified_at\": {} }}",
                     profile.template_id, profile.name, profile.is_admin ? "true" : "false", created_time,
                     modified_time);
#else
  std::ostringstream oss;
  oss << "{ \"template_id\": " << profile.template_id << ", \"name\": \"" << profile.name
      << "\", \"is_admin\": " << (profile.is_admin ? "true" : "false") << ", \"created_at\": " << created_time
      << ", \"modified_at\": " << modified_time << " }";
  return oss.str();
#endif
}

} // namespace carbio
