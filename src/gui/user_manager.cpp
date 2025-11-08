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

#include "user_manager.h"
#include <QDateTime>

UserManager::UserManager(carbio::TemplateMetadataStore *store, QObject *parent)
    : QAbstractListModel(parent), m_store(store) {
    syncFromStore();
}

void UserManager::syncFromStore() {
    beginResetModel();
    m_users = m_store->get_all();
    endResetModel();
    emit countChanged();
}

int UserManager::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_users.size());
}

QVariant UserManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_users.size())) {
        return QVariant();
    }

    const carbio::TemplateMetadata &user = m_users[static_cast<size_t>(index.row())];

    switch (role) {
    case IdRole:
        return user.template_id;
    case NameRole:
        return QString::fromStdString(user.display_name);
    case RoleNameRole:
        return roleToString(user.role);
    case RoleColorRole:
        return roleToColor(user.role);
    case EnrolledAtRole:
        return formatTimestamp(user.enrolled_at);
    case LastAccessRole:
        if (user.last_access_at.has_value()) {
            return formatTimestamp(user.last_access_at.value());
        }
        return QString("Never");
    case IsAdminRole:
        return user.is_admin();
    case StateRole:
        return static_cast<int>(user.state);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> UserManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[RoleNameRole] = "roleName";
    roles[RoleColorRole] = "roleColor";
    roles[EnrolledAtRole] = "enrolledAt";
    roles[LastAccessRole] = "lastAccess";
    roles[IsAdminRole] = "isAdmin";
    roles[StateRole] = "state";
    return roles;
}

int UserManager::addUser(const QString &name, int roleType) {
    if (name.isEmpty()) {
        return -1;
    }

    int id = findNextAvailableId();
    if (id < 0 || id > 127) {
        return -1;
    }

    carbio::TemplateMetadata meta{};
    meta.template_id = static_cast<uint16_t>(id);
    meta.role = static_cast<carbio::Role>(roleType);
    meta.state = carbio::RoleState::PENDING;  // Will become ACTIVE after enrollment
    meta.display_name = name.toStdString();
    meta.enrolled_at = std::chrono::system_clock::now();
    meta.last_access_at = std::nullopt;
    meta.expires_at = std::nullopt;
    meta.successful_auth_count = 0;
    meta.failed_auth_count = 0;
    meta.notes = "";

    m_store->store(meta);
    refresh();
    emit userAdded(id);

    return id;
}

bool UserManager::removeUser(int templateId) {
    if (!m_store->remove(static_cast<uint16_t>(templateId))) {
        return false;
    }

    refresh();
    emit userRemoved(templateId);
    return true;
}

bool UserManager::updateUser(int templateId, const QString &name, int roleType) {
    auto meta = m_store->get(static_cast<uint16_t>(templateId));
    if (!meta.has_value()) {
        return false;
    }

    meta->display_name = name.toStdString();
    meta->role = static_cast<carbio::Role>(roleType);

    m_store->store(*meta);
    refresh();
    emit userUpdated(templateId);

    return true;
}

QVariantMap UserManager::getUserInfo(int templateId) const {
    auto meta = m_store->get(static_cast<uint16_t>(templateId));
    if (!meta.has_value()) {
        return QVariantMap();
    }

    QVariantMap info;
    info["id"] = meta->template_id;
    info["name"] = QString::fromStdString(meta->display_name);
    info["roleName"] = roleToString(meta->role);
    info["roleType"] = static_cast<int>(meta->role);
    info["roleColor"] = roleToColor(meta->role);
    info["isAdmin"] = meta->is_admin();
    info["enrolledAt"] = formatTimestamp(meta->enrolled_at);

    return info;
}

int UserManager::findNextAvailableId() const {
    // Notepad supports only 16 templates (IDs 0-15)
    // Each notepad page corresponds to one template ID
    for (int id = 0; id <= 15; ++id) {
        if (isIdAvailable(id)) {
            return id;
        }
    }
    return -1;  // No available IDs (notepad full)
}

bool UserManager::isIdAvailable(int id) const {
    // Notepad only supports IDs 0-15
    if (id < 0 || id > 15) {
        return false;
    }

    return !m_store->get(static_cast<uint16_t>(id)).has_value();
}

QString UserManager::getRoleColor(int roleType) const {
    return roleToColor(static_cast<carbio::Role>(roleType));
}

QString UserManager::getRoleName(int roleType) const {
    return roleToString(static_cast<carbio::Role>(roleType));
}

void UserManager::refresh() {
    syncFromStore();
}

QString UserManager::roleToString(carbio::Role role) const {
    return QString::fromStdString(std::string(carbio::role_name(role)));
}

QString UserManager::roleToColor(carbio::Role role) const {
    switch (role) {
    case carbio::Role::PrimaryOwner:
        return "#FFD700";  // Gold
    case carbio::Role::SecondaryOwner:
        return "#01E4E0";  // Cyan
    case carbio::Role::FamilyMember:
        return "#32D74B";  // Green
    case carbio::Role::RestrictedDriver:
        return "#FFB340";  // Orange
    case carbio::Role::Passenger:
        return "#0A84FF";  // Blue
    case carbio::Role::ServiceTech:
        return "#BF5AF2";  // Purple
    default:
        return "#888888";  // Gray
    }
}

QString UserManager::formatTimestamp(const std::chrono::system_clock::time_point &tp) const {
    auto timestamp = std::chrono::system_clock::to_time_t(tp);
    QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp));
    return dt.toString("yyyy-MM-dd hh:mm");
}
