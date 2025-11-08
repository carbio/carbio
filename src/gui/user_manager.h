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
#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <vector>

/**
 * @brief Qt model for managing user fingerprint templates
 *
 * Provides a QML-friendly interface to the TemplateMetadataStore.
 * Displays enrolled users with names, roles, and last access times.
 */
class UserManager final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum UserRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        RoleNameRole,
        RoleColorRole,
        EnrolledAtRole,
        LastAccessRole,
        IsAdminRole,
        StateRole
    };

    explicit UserManager(carbio::TemplateMetadataStore *store, QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // User management
    Q_INVOKABLE int addUser(const QString &name, int roleType);
    Q_INVOKABLE bool removeUser(int templateId);
    Q_INVOKABLE bool updateUser(int templateId, const QString &name, int roleType);
    Q_INVOKABLE QVariantMap getUserInfo(int templateId) const;

    // Query
    Q_INVOKABLE int findNextAvailableId() const;
    Q_INVOKABLE bool isIdAvailable(int id) const;
    Q_INVOKABLE QString getRoleColor(int roleType) const;
    Q_INVOKABLE QString getRoleName(int roleType) const;

    int count() const { return static_cast<int>(m_users.size()); }

public slots:
    void refresh();

signals:
    void countChanged();
    void userAdded(int templateId);
    void userRemoved(int templateId);
    void userUpdated(int templateId);

private:
    carbio::TemplateMetadataStore *m_store;  // Non-owning pointer
    std::vector<carbio::TemplateMetadata> m_users;

    void syncFromStore();
    QString roleToString(carbio::Role role) const;
    QString roleToColor(carbio::Role role) const;
    QString formatTimestamp(const std::chrono::system_clock::time_point &tp) const;
};
