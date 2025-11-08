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

#include "admin_session.h"

AdminSession::AdminSession(QObject *parent)
    : QObject(parent),
      m_isActive(false),
      m_totalDuration(300),
      m_timer(new QTimer(this)) {
    m_timer->setInterval(TICK_INTERVAL_MS);
    connect(m_timer, &QTimer::timeout, this, &AdminSession::onTick);
}

void AdminSession::startSession(int durationSeconds) {
    if (durationSeconds <= 0) {
        return;
    }

    m_totalDuration = durationSeconds;
    m_expiryTime = std::chrono::steady_clock::now() +
                   std::chrono::seconds(durationSeconds);

    if (!m_isActive) {
        m_isActive = true;
        emit isActiveChanged();
        emit sessionStarted();
    }

    m_timer->start();
    emit remainingSecondsChanged();
}

void AdminSession::endSession() {
    if (!m_isActive) {
        return;
    }

    m_timer->stop();
    m_isActive = false;
    emit isActiveChanged();
    emit remainingSecondsChanged();
}

void AdminSession::extendSession(int additionalSeconds) {
    if (!m_isActive || additionalSeconds <= 0) {
        return;
    }

    m_expiryTime += std::chrono::seconds(additionalSeconds);
    emit remainingSecondsChanged();
}

int AdminSession::remainingSeconds() const {
    if (!m_isActive) {
        return 0;
    }

    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        m_expiryTime - now
    );

    return std::max(0, static_cast<int>(remaining.count()));
}

void AdminSession::onTick() {
    int remaining = remainingSeconds();

    if (remaining <= 0) {
        m_timer->stop();
        m_isActive = false;
        emit sessionExpired();
        emit isActiveChanged();
    }

    emit remainingSecondsChanged();
}
