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

#include "authentication_manager.h"
#include <QDebug>

AuthenticationManager::AuthenticationManager(QObject *parent)
    : QObject(parent),
      m_authState(AuthState::Off),
      m_failedAttempts(0),
      m_lockoutSeconds(0),
      m_currentUserId(-1),
      m_lockoutTimer(new QTimer(this)) {

    m_lockoutTimer->setInterval(1000);  // 1 second tick
    connect(m_lockoutTimer, &QTimer::timeout, this, &AuthenticationManager::onLockoutTick);
}

void AuthenticationManager::startScanning() {
    if (m_authState == AuthState::Alert) {
        qWarning() << "[AuthManager] Cannot start scanning - system is locked out";
        return;
    }

    setState(AuthState::Scanning);
    emit scanningStarted();
}

void AuthenticationManager::recordSuccess(int userId) {
    qInfo() << "[AuthManager] Authentication successful for user ID:" << userId;

    // Reset failed attempts
    if (m_failedAttempts != 0) {
        m_failedAttempts = 0;
        emit failedAttemptsChanged();
    }

    // Track current user
    if (m_currentUserId != userId) {
        m_currentUserId = userId;
        emit currentUserChanged();
    }

    setState(AuthState::On);
    emit authenticationSuccess(userId);
}

void AuthenticationManager::recordFailure() {
    qWarning() << "[AuthManager] Authentication failed";

    m_failedAttempts++;
    emit failedAttemptsChanged();
    emit authenticationFailed();

    if (shouldLockout()) {
        enterLockout();
    } else {
        // Stay in scanning state for retry
        setState(AuthState::Scanning);
    }
}

void AuthenticationManager::recordNoFinger() {
    // Silent - no state change needed
    // Worker thread handles polling, this is just for logging
}

void AuthenticationManager::resetFailedAttempts() {
    if (m_failedAttempts != 0) {
        qInfo() << "[AuthManager] Resetting failed attempts from" << m_failedAttempts << "to 0";
        m_failedAttempts = 0;
        emit failedAttemptsChanged();
    }
}

void AuthenticationManager::logout() {
    qInfo() << "[AuthManager] User logged out";
    m_currentUserId = -1;
    emit currentUserChanged();
    setState(AuthState::Off);
}

void AuthenticationManager::setState(AuthState state) {
    if (m_authState != state) {
        qInfo() << "[AuthManager] State transition:"
                << static_cast<int>(m_authState) << "→" << static_cast<int>(state);
        m_authState = state;
        emit authStateChanged();
    }
}

void AuthenticationManager::enterLockout() {
    qWarning() << "[AuthManager] Max attempts reached - entering lockout";

    m_lockoutSeconds = LOCKOUT_DURATION_SEC;
    emit lockoutSecondsChanged();

    setState(AuthState::Alert);
    m_lockoutTimer->start();

    emit lockoutTriggered();
}

bool AuthenticationManager::shouldLockout() const {
    return m_failedAttempts >= MAX_ATTEMPTS;
}

void AuthenticationManager::onLockoutTick() {
    if (m_lockoutSeconds > 0) {
        m_lockoutSeconds--;
        emit lockoutSecondsChanged();

        if (m_lockoutSeconds <= 0) {
            qInfo() << "[AuthManager] Lockout expired - returning to scanning";
            m_lockoutTimer->stop();

            // Reset failed attempts
            m_failedAttempts = 0;
            emit failedAttemptsChanged();

            setState(AuthState::Scanning);
            emit lockoutExpired();
        }
    }
}
