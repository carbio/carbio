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
#include <QObject>
#include <QString>

class UserManager;

/**
 * @brief Manages fingerprint enrollment process
 *
 * Single Responsibility: Enrollment orchestration
 * - Validates enrollment requests
 * - Manages pending user data
 * - Ensures atomic user creation
 * - Handles enrollment cancellation
 */
class EnrollmentManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isEnrolling READ isEnrolling NOTIFY isEnrollingChanged)
    Q_PROPERTY(QString enrollmentStatus READ enrollmentStatus NOTIFY enrollmentStatusChanged)

public:
    explicit EnrollmentManager(carbio::TemplateMetadataStore *metadataStore,
                              UserManager *userManager,
                              QObject *parent = nullptr);

    // Property getters
    bool isEnrolling() const { return m_isEnrolling; }
    QString enrollmentStatus() const { return m_enrollmentStatus; }

    // Enrollment operations
    bool startFirstBootEnrollment(const QString &userName, int roleType);
    bool startEnrollment(int userId);
    void cancelEnrollment();

    // Callbacks from sensor worker
    void onEnrollmentComplete(int userId);
    void onEnrollmentFailed(const QString &error);

signals:
    // State signals
    void isEnrollingChanged();
    void enrollmentStatusChanged();

    // Flow signals
    void enrollmentRequested(int userId);  // Signal to start sensor enrollment
    void enrollmentSuccess(int userId, QString message);
    void enrollmentError(QString error);
    void enrollmentCancelled();

private:
    struct PendingEnrollment {
        int userId;
        QString userName;
        int roleType;
        bool isFirstBoot;
    };

    bool validateEnrollmentRequest(int userId);
    void setEnrolling(bool enrolling);
    void setStatus(const QString &status);
    void clearPendingEnrollment();
    void createUser();

    carbio::TemplateMetadataStore *m_metadataStore;  // Non-owning
    UserManager *m_userManager;                      // Non-owning

    bool m_isEnrolling;
    QString m_enrollmentStatus;
    PendingEnrollment m_pending;
};
