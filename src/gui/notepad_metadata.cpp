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

#include "notepad_metadata.h"
#include <cstring>

QByteArray NotepadMetadata::serialize() const {
    QByteArray data(32, 0x00);  // 32 bytes, zero-initialized

    // Bytes 0-3: Enrolled timestamp (little-endian)
    data[0] = static_cast<char>(enrolled_at & 0xFF);
    data[1] = static_cast<char>((enrolled_at >> 8) & 0xFF);
    data[2] = static_cast<char>((enrolled_at >> 16) & 0xFF);
    data[3] = static_cast<char>((enrolled_at >> 24) & 0xFF);

    // Bytes 4-7: Last access timestamp (little-endian)
    data[4] = static_cast<char>(last_access_at & 0xFF);
    data[5] = static_cast<char>((last_access_at >> 8) & 0xFF);
    data[6] = static_cast<char>((last_access_at >> 16) & 0xFF);
    data[7] = static_cast<char>((last_access_at >> 24) & 0xFF);

    // Bytes 8-11: Metadata
    data[8] = static_cast<char>(template_id);
    data[9] = static_cast<char>(role);
    data[10] = static_cast<char>(state);
    data[11] = static_cast<char>(reserved);

    // Bytes 12-31: Display name (20 bytes, null-terminated)
    // Copy at most 19 characters, then ensure null termination
    const size_t name_len = std::strlen(display_name);
    const size_t copy_len = (name_len < 19) ? name_len : 19;
    std::memcpy(data.data() + 12, display_name, copy_len);
    data[12 + copy_len] = '\0';  // Null terminate at correct position
    // Zero-fill remaining bytes
    if (copy_len < 19) {
        std::memset(data.data() + 12 + copy_len + 1, 0, 19 - copy_len - 1);
    }

    return data;
}

std::optional<NotepadMetadata> NotepadMetadata::deserialize(const QByteArray& data) {
    if (data.size() != 32) {
        return std::nullopt;
    }

    NotepadMetadata meta;

    // Bytes 0-3: Enrolled timestamp (little-endian)
    meta.enrolled_at = static_cast<uint32_t>(static_cast<uint8_t>(data[0])) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 8) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 24);

    // Bytes 4-7: Last access timestamp (little-endian)
    meta.last_access_at = static_cast<uint32_t>(static_cast<uint8_t>(data[4])) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(data[5])) << 8) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(data[6])) << 16) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(data[7])) << 24);

    // Bytes 8-11: Metadata
    meta.template_id = static_cast<uint8_t>(data[8]);
    meta.role = static_cast<uint8_t>(data[9]);
    meta.state = static_cast<uint8_t>(data[10]);
    meta.reserved = static_cast<uint8_t>(data[11]);

    // Bytes 12-31: Display name (ensure null termination)
    std::strncpy(meta.display_name, data.constData() + 12, 19);
    meta.display_name[19] = '\0';

    return meta;
}

NotepadMetadata NotepadMetadata::fromTemplateMetadata(const carbio::TemplateMetadata& meta) {
    NotepadMetadata notepad;

    // Convert chrono timestamps to Unix timestamps
    notepad.enrolled_at = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            meta.enrolled_at.time_since_epoch()).count());

    if (meta.last_access_at.has_value()) {
        notepad.last_access_at = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                meta.last_access_at.value().time_since_epoch()).count());
    } else {
        notepad.last_access_at = 0;  // Never accessed
    }

    notepad.template_id = static_cast<uint8_t>(meta.template_id);
    notepad.role = static_cast<uint8_t>(meta.role);
    notepad.state = static_cast<uint8_t>(meta.state);
    notepad.reserved = 0;

    // Truncate name to 19 chars (+ null terminator)
    std::string truncated_name = meta.display_name.substr(0, 19);
    std::strncpy(notepad.display_name, truncated_name.c_str(), 19);
    notepad.display_name[19] = '\0';

    return notepad;
}

carbio::TemplateMetadata NotepadMetadata::toTemplateMetadata() const {
    carbio::TemplateMetadata meta{};

    meta.template_id = static_cast<uint16_t>(template_id);
    meta.role = static_cast<carbio::Role>(role);
    meta.state = static_cast<carbio::RoleState>(state);
    meta.display_name = std::string(display_name);

    // Convert Unix timestamps to chrono time_points
    meta.enrolled_at = std::chrono::system_clock::from_time_t(
        static_cast<std::time_t>(enrolled_at));

    if (last_access_at != 0) {
        meta.last_access_at = std::chrono::system_clock::from_time_t(
            static_cast<std::time_t>(last_access_at));
    } else {
        meta.last_access_at = std::nullopt;
    }

    meta.expires_at = std::nullopt;
    meta.successful_auth_count = 0;
    meta.failed_auth_count = 0;
    meta.notes = "Restored from sensor notepad";

    return meta;
}

bool NotepadMetadata::isValid(const QByteArray& data) {
    if (data.size() != 32) {
        return false;
    }

    // Check if all bytes are zero (empty page)
    bool all_zero = true;
    for (int i = 0; i < 32; ++i) {
        if (data[i] != 0) {
            all_zero = false;
            break;
        }
    }

    // Empty page is considered invalid
    if (all_zero) {
        return false;
    }

    // Valid if non-zero and correct size
    return true;
}
