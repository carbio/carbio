/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
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

#pragma once

#include "template_metadata.h"
#include <QByteArray>
#include <QString>
#include <array>
#include <cstdint>

/**
 * @brief Compact metadata format for sensor notepad storage
 *
 * Layout per user (32 bytes - perfect fit):
 * - Bytes 0-3: Enrolled timestamp (Unix 32-bit, seconds since epoch)
 * - Bytes 4-7: Last access timestamp (Unix 32-bit, 0 = never accessed)
 * - Byte 8: Template ID (0-127)
 * - Byte 9: Role (enum carbio::Role)
 * - Byte 10: State (enum carbio::RoleState)
 * - Byte 11: Reserved (unused, for future extensions)
 * - Bytes 12-31: Display name (20 bytes = 19 chars + null terminator)
 *
 * Page organization (32 bytes per page):
 * - Each page stores 1 user (32 bytes exactly, zero waste)
 * - Page number = template_id (direct mapping)
 * - Offset = 0 (always use entire page)
 * - Total capacity: 16 pages × 1 user = 16 users
 */
struct NotepadMetadata {
    static constexpr size_t BYTES_PER_USER = 32;
    static constexpr size_t USERS_PER_PAGE = 1;
    static constexpr size_t MAX_USERS = 16;
    static constexpr size_t NAME_LENGTH = 20;  // 19 chars + null

    uint32_t enrolled_at{0};      // Unix timestamp (seconds since 1970-01-01)
    uint32_t last_access_at{0};   // Unix timestamp (0 = never accessed)
    uint8_t template_id{0};
    uint8_t role{0};              // carbio::Role
    uint8_t state{0};             // carbio::RoleState
    uint8_t reserved{0};          // Reserved for future use
    char display_name[NAME_LENGTH]{};  // 19 chars + null terminator

    /**
     * @brief Serialize to 32-byte array for notepad write
     */
    [[nodiscard]] QByteArray serialize() const;

    /**
     * @brief Deserialize from 32-byte notepad data
     * @return true if valid (magic marker matches), false otherwise
     */
    [[nodiscard]] static std::optional<NotepadMetadata> deserialize(const QByteArray& data);

    /**
     * @brief Create from TemplateMetadata (extract essential fields)
     */
    [[nodiscard]] static NotepadMetadata fromTemplateMetadata(const carbio::TemplateMetadata& meta);

    /**
     * @brief Convert to TemplateMetadata (fill with defaults for missing fields)
     */
    [[nodiscard]] carbio::TemplateMetadata toTemplateMetadata() const;

    /**
     * @brief Check if notepad data is valid (has correct magic marker)
     */
    [[nodiscard]] static bool isValid(const QByteArray& data);
};
