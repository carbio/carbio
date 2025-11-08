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

#include "enrollment_manager.h"
#include "user_manager.h"
#include <QDebug>

EnrollmentManager::EnrollmentManager(carbio::TemplateMetadataStore *metadataStore,
                                     UserManager *userManager,
                                     QObject *parent)
    : QObject(parent),
      m_metadataStore(metadataStore),
      m_userManager(userManager),
      m_isEnrolling(false),
      m_enrollmentStatus(""),
      m_pending{-1, "", 0, false} {
}

bool EnrollmentManager::startFirstBootEnrollment(const QString &userName, int roleType) {
    qInfo() << "[EnrollmentMgr] First-boot enrollment requested for:" << userName;

    if (userName.isEmpty()) {
        qWarning() << "[EnrollmentMgr] Cannot enroll with empty name";
        emit enrollmentError("Name cannot be empty");
        return false;
    }

    if (m_isEnrolling) {
        qWarning() << "[EnrollmentMgr] Enrollment already in progress";
        emit enrollmentError("Enrollment already in progress");
        return false;
    }

    // Reserve ID 0 for first user (Primary Owner)
    int userId = 0;

    // Store pending enrollment (user created ONLY on success)
    m_pending.userId = userId;
    m_pending.userName = userName;
    m_pending.roleType = roleType;
    m_pending.isFirstBoot = true;

    setEnrolling(true);
    setStatus("Initializing enrollment...");

    qInfo() << "[EnrollmentMgr] Starting first-boot enrollment for ID:" << userId;
    emit enrollmentRequested(userId);

    return true;
}

bool EnrollmentManager::startEnrollment(int userId) {
    qInfo() << "[EnrollmentMgr] Enrollment requested for existing user ID:" << userId;

    if (!validateEnrollmentRequest(userId)) {
        return false;
    }

    if (m_isEnrolling) {
        qWarning() << "[EnrollmentMgr] Enrollment already in progress";
        emit enrollmentError("Enrollment already in progress");
        return false;
    }

    // Metadata state will be updated to ACTIVE on enrollment success
    m_pending.userId = userId;
    m_pending.isFirstBoot = false;

    setEnrolling(true);
    setStatus("Initializing enrollment...");

    qInfo() << "[EnrollmentMgr] Starting enrollment for ID:" << userId;
    emit enrollmentRequested(userId);

    return true;
}

void EnrollmentManager::cancelEnrollment() {
    if (!m_isEnrolling) {
        qWarning() << "[EnrollmentMgr] No active enrollment to cancel";
        return;
    }

    qWarning() << "[EnrollmentMgr] Enrollment cancelled by user";

    if (m_pending.isFirstBoot) {
        qInfo() << "[EnrollmentMgr] First-boot enrollment cancelled for:" << m_pending.userName;
    }

    clearPendingEnrollment();
    setEnrolling(false);
    setStatus("Enrollment cancelled");

    emit enrollmentCancelled();
}

void EnrollmentManager::onEnrollmentComplete(int userId) {
    qInfo() << "[EnrollmentMgr] Enrollment complete for user ID:" << userId;

    if (!m_isEnrolling) {
        qWarning() << "[EnrollmentMgr] Received completion but not enrolling";
        return;
    }

    // ATOMIC: Create user ONLY now that fingerprint is enrolled
    if (m_pending.isFirstBoot) {
        createUser();
    }

    setEnrolling(false);
    setStatus("Enrollment complete");

    QString message = QString("User ID %1 enrolled successfully").arg(userId);
    emit enrollmentSuccess(userId, message);

    clearPendingEnrollment();
}

void EnrollmentManager::onEnrollmentFailed(const QString &error) {
    qWarning() << "[EnrollmentMgr] Enrollment failed:" << error;

    if (!m_isEnrolling) {
        qWarning() << "[EnrollmentMgr] Received failure but not enrolling";
        return;
    }

    if (m_pending.isFirstBoot) {
        qWarning() << "[EnrollmentMgr] First-boot enrollment failed for:" << m_pending.userName;
    }

    clearPendingEnrollment();
    setEnrolling(false);
    setStatus("Enrollment failed");

    emit enrollmentError(error);
}

bool EnrollmentManager::validateEnrollmentRequest(int userId) {
    // Notepad supports only IDs 0-15 (16 pages)
    if (userId < 0 || userId > 15) {
        qWarning() << "[EnrollmentMgr] Invalid user ID:" << userId;
        emit enrollmentError("Invalid user ID. Notepad supports only IDs 0-15.");
        return false;
    }

    return true;
}

void EnrollmentManager::setEnrolling(bool enrolling) {
    if (m_isEnrolling != enrolling) {
        m_isEnrolling = enrolling;
        emit isEnrollingChanged();
    }
}

void EnrollmentManager::setStatus(const QString &status) {
    if (m_enrollmentStatus != status) {
        m_enrollmentStatus = status;
        emit enrollmentStatusChanged();
    }
}

void EnrollmentManager::clearPendingEnrollment() {
    m_pending.userId = -1;
    m_pending.userName.clear();
    m_pending.roleType = 0;
    m_pending.isFirstBoot = false;
}

void EnrollmentManager::createUser() {
    qInfo() << "[EnrollmentMgr] Creating first-boot user:" << m_pending.userName
            << "with role:" << m_pending.roleType;

    if (!m_userManager) {
        qCritical() << "[EnrollmentMgr] CRITICAL: UserManager is null!";
        return;
    }

    // Create user in UserManager (this will be written to notepad via userAdded signal)
    int userId = m_userManager->addUser(m_pending.userName, m_pending.roleType);

    if (userId >= 0) {
        qInfo() << "[EnrollmentMgr] First-boot user created successfully with ID:" << userId;
        // Note: Metadata will be written to notepad automatically via Controller's signal handler
    } else {
        qWarning() << "[EnrollmentMgr] Failed to create user in UserManager";
    }
}
