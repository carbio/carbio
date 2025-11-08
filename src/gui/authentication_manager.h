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

#include "auth_types.h"
#include <QObject>
#include <QTimer>

/**
 * @brief Manages authentication state machine and lockout logic
 *
 * Single Responsibility: Authentication flow only
 * - State transitions (Off → Scanning → Authenticating → Alert → On)
 * - Failed attempt tracking
 * - Lockout timer management
 * - User session tracking
 */
class AuthenticationManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(AuthState authState READ authState NOTIFY authStateChanged)
    Q_PROPERTY(int failedAttempts READ failedAttempts NOTIFY failedAttemptsChanged)
    Q_PROPERTY(int lockoutSeconds READ lockoutSeconds NOTIFY lockoutSecondsChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserChanged)

public:
    explicit AuthenticationManager(QObject *parent = nullptr);

    // Property getters
    AuthState authState() const { return m_authState; }
    int failedAttempts() const { return m_failedAttempts; }
    int lockoutSeconds() const { return m_lockoutSeconds; }
    int currentUserId() const { return m_currentUserId; }

    // State queries
    bool isScanning() const { return m_authState == AuthState::Scanning; }
    bool isAuthenticating() const { return m_authState == AuthState::Authenticating; }
    bool isLocked() const { return m_authState == AuthState::Alert; }
    bool isAuthenticated() const { return m_authState == AuthState::On; }

    // Authentication flow control
    void startScanning();
    void recordSuccess(int userId);
    void recordFailure();
    void recordNoFinger();
    void resetFailedAttempts();
    void logout();

signals:
    // Property change signals
    void authStateChanged();
    void failedAttemptsChanged();
    void lockoutSecondsChanged();
    void currentUserChanged();

    // Flow signals
    void authenticationSuccess(int userId);
    void authenticationFailed();
    void lockoutTriggered();
    void lockoutExpired();
    void scanningStarted();

private slots:
    void onLockoutTick();

private:
    void setState(AuthState state);
    void enterLockout();
    bool shouldLockout() const;

    // State
    AuthState m_authState;
    int m_failedAttempts;
    int m_lockoutSeconds;
    int m_currentUserId;

    // Lockout timer
    QTimer *m_lockoutTimer;

    // Constants
    static constexpr int MAX_ATTEMPTS = 3;
    static constexpr int LOCKOUT_DURATION_SEC = 20;
};
