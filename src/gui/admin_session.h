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

#include <QObject>
#include <QTimer>
#include <chrono>

/**
 * @brief Manages admin access session with automatic timeout
 *
 * Once admin authentication succeeds, creates a time-limited session
 * so user doesn't need to re-authenticate for every settings access.
 * Default session duration is 5 minutes (300 seconds).
 */
class AdminSession final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(int remainingSeconds READ remainingSeconds NOTIFY remainingSecondsChanged)
    Q_PROPERTY(int totalSeconds READ totalSeconds CONSTANT)

public:
    explicit AdminSession(QObject *parent = nullptr);
    ~AdminSession() override = default;

    /**
     * @brief Start a new admin session
     * @param durationSeconds Session duration in seconds (default: 300 = 5 minutes)
     */
    Q_INVOKABLE void startSession(int durationSeconds = 300);

    /**
     * @brief End the current session immediately
     */
    Q_INVOKABLE void endSession();

    /**
     * @brief Extend the current session by specified seconds
     * @param additionalSeconds Seconds to add to remaining time
     */
    Q_INVOKABLE void extendSession(int additionalSeconds = 60);

    /**
     * @brief Check if session is currently active
     */
    [[nodiscard]] bool isActive() const { return m_isActive; }

    /**
     * @brief Get remaining seconds in current session
     * @return Seconds remaining, or 0 if no active session
     */
    [[nodiscard]] int remainingSeconds() const;

    /**
     * @brief Get total session duration
     */
    [[nodiscard]] int totalSeconds() const { return m_totalDuration; }

signals:
    void isActiveChanged();
    void remainingSecondsChanged();
    void sessionExpired();
    void sessionStarted();

private slots:
    void onTick();

private:
    bool m_isActive;
    int m_totalDuration;
    QTimer *m_timer;
    std::chrono::steady_clock::time_point m_expiryTime;

    static constexpr int TICK_INTERVAL_MS = 1000;  // Update every second
};
