// Copyright (c) 2024 CARBIO Project
// SPDX-License-Identifier: MIT

#ifndef CARBIO_CLI_TEMPLATE_METADATA_H
#define CARBIO_CLI_TEMPLATE_METADATA_H

#include "role.h"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace carbio {

/**
 * @brief Role lifecycle states
 *
 * According to the RBAC lifecycle management section in ch2.tex.
 */
enum class RoleState : uint8_t {
    PENDING   = 0,  ///< Pending, not yet activated
    ACTIVE    = 1,  ///< Active, usable
    SUSPENDED = 2,  ///< Suspended, temporarily disabled
    EXPIRED   = 3,  ///< Expired (time-limited role)
    REVOKED   = 4   ///< Revoked, permanently disabled
};

/**
 * @brief Fingerprint template metadata structure
 *
 * This structure stores metadata for a fingerprint template,
 * including RBAC role, state, timestamps, and user data.
 *
 * According to the system design in ch3.tex, this replaces
 * the hardcoded admin ID range (0-2) check.
 */
struct TemplateMetadata {
    /**
     * @brief Template ID (0-127 for Adafruit ID751)
     */
    uint16_t template_id;

    /**
     * @brief RBAC role (R1-R6)
     */
    Role role;

    /**
     * @brief Role state (active, suspended, etc.)
     */
    RoleState state;

    /**
     * @brief User display name
     *
     * NOTE: Uses std::string (heap allocation) because we need to own
     * the string data. string_view cannot be used here as the source
     * data (from notepad or QString) may be destroyed, leaving dangling pointers.
     */
    std::string display_name;

    /**
     * @brief Enrollment timestamp
     */
    std::chrono::system_clock::time_point enrolled_at;

    /**
     * @brief Last access timestamp
     */
    std::optional<std::chrono::system_clock::time_point> last_access_at;

    /**
     * @brief Expiration time (for time-limited roles like R4)
     */
    std::optional<std::chrono::system_clock::time_point> expires_at;

    /**
     * @brief Successful authentication count
     */
    uint32_t successful_auth_count = 0;

    /**
     * @brief Failed authentication count
     */
    uint32_t failed_auth_count = 0;

    /**
     * @brief Notes (optional)
     *
     * NOTE: Uses std::string (heap allocation) because we need to own
     * the string data. string_view cannot be used here as the source
     * data (from notepad or QString) may be destroyed, leaving dangling pointers.
     */
    std::string notes;

    // Out-of-line special members to avoid -Winline warnings (std::string members)
    TemplateMetadata() = default;
    TemplateMetadata(const TemplateMetadata&);
    TemplateMetadata& operator=(const TemplateMetadata&);
    TemplateMetadata(TemplateMetadata&&) noexcept;
    TemplateMetadata& operator=(TemplateMetadata&&) noexcept;
    ~TemplateMetadata();

    /**
     * @brief Check if the role has admin privileges
     *
     * This is the secure way to determine if someone is admin.
     * Not based on ID (hardcoded 0-2), but on metadata.
     *
     * @return true if has AdminAccess permission
     */
    [[nodiscard]] bool is_admin() const {
        return state == RoleState::ACTIVE
            && has_permission(role, Permission::AdminAccess);
    }

    /**
     * @brief Check if the role is active and usable
     *
     * Takes into account state and possible expiration.
     *
     * @return true if usable
     */
    [[nodiscard]] bool is_usable() const {
        if (state != RoleState::ACTIVE) {
            return false;
        }

        // Check expiration
        if (expires_at.has_value()) {
            auto now = std::chrono::system_clock::now();
            if (now >= expires_at.value()) {
                return false;  // Expired
            }
        }

        return true;
    }

    /**
     * @brief Check if a specific permission can be used
     *
     * @param perm The permission
     * @return true if the role is active and has the permission
     */
    [[nodiscard]] bool can_perform(Permission perm) const {
        return is_usable() && has_permission(role, perm);
    }
};

/**
 * @brief Template metadata manager class
 *
 * Stores template metadata in memory and provides
 * CRUD operations. A later version could add persistent storage
 * (JSON/SQLite).
 */
class TemplateMetadataStore {
public:
    /**
     * @brief Default constructor
     */
    TemplateMetadataStore() = default;

    /**
     * @brief Add or update metadata
     *
     * @param metadata The metadata
     */
    void store(const TemplateMetadata& metadata) {
        store_[metadata.template_id] = metadata;
    }

    /**
     * @brief Get metadata by template ID
     *
     * @param template_id The template ID
     * @return The metadata or std::nullopt if not found
     */
    [[nodiscard]] std::optional<TemplateMetadata> get(uint16_t template_id) const {
        auto it = store_.find(template_id);
        if (it != store_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Delete metadata
     *
     * @param template_id The template ID
     * @return true if successfully deleted, false if didn't exist
     */
    bool remove(uint16_t template_id) {
        return store_.erase(template_id) > 0;
    }

    /**
     * @brief Get all metadata
     *
     * @return Vector with metadata
     */
    [[nodiscard]] std::vector<TemplateMetadata> get_all() const {
        std::vector<TemplateMetadata> result;
        result.reserve(store_.size());
        for (const auto& [id, metadata] : store_) {
            result.push_back(metadata);
        }
        return result;
    }

    /**
     * @brief Count of active templates
     *
     * @return The count of active templates
     */
    [[nodiscard]] size_t active_count() const {
        size_t count = 0;
        for (const auto& [id, metadata] : store_) {
            if (metadata.is_usable()) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Count of admin templates
     *
     * @return The count of templates with admin privileges
     */
    [[nodiscard]] size_t admin_count() const {
        size_t count = 0;
        for (const auto& [id, metadata] : store_) {
            if (metadata.is_admin()) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Delete all templates
     */
    void clear() {
        store_.clear();
    }

    /**
     * @brief Count of templates
     *
     * @return The count of templates
     */
    [[nodiscard]] size_t size() const {
        return store_.size();
    }

    /**
     * @brief Is the store empty
     *
     * @return true if empty
     */
    [[nodiscard]] bool empty() const {
        return store_.empty();
    }
    
private:
    std::unordered_map<uint16_t, TemplateMetadata> store_;
};

}  // namespace carbio

#endif  // CARBIO_CLI_TEMPLATE_METADATA_H
