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

#include "carbio/fingerprint/device_setting_info.h"
#include "carbio/utility/blocking_queue.h"
#include "carbio/utility/lockfree_triple_buffer.h"
#include "carbio/utility/secure_value.h"

#include "sensor_command_queue.h"
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>
#include <optional>

namespace carbio::fingerprint {
class fingerprint_sensor;
struct search_query_info;
struct match_query_info;
} // namespace carbio::fingerprint

class SensorWorker final : public QObject {
  Q_OBJECT

public:
  explicit SensorWorker(carbio::fingerprint::fingerprint_sensor *sensor,
                        QObject *parent = nullptr);
  ~SensorWorker() override;

public slots:
  // Simple polling control (direct authentication polling)
  void startAuthenticationPolling(
      int intervalMs); // Start authentication polling at specified interval
  void stopAuthenticationPolling(); // Stop authentication polling

  // Admin authentication polling
  void startAdminPolling(int intervalMs); // Start admin auth polling
  void stopAdminPolling();                // Stop admin auth polling

  // Performance: Pre-warm cache on startup
  void prewarmCache();

  // Single-shot authentication (for dialog operations)
  void performAuthentication();
  void performAdminAuthentication();

  // Enrollment and identification
  void enrollFingerprint(int id);
  void findFingerprint();
  void identifyFingerprint();
  void verifyFingerprint(int id);
  void queryTemplate(int id);

  // Database operations
  void deleteFingerprint(int id);
  void clearDatabase();
  void refreshTemplateCount();

  // LED operations
  void turnLedOn();
  void turnLedOff();

  // Configuration operations
  void setBaudRate(int baudChoice);
  void setSecurityLevel(int level);
  void setPacketSize(int size);
  void softResetSensor();
  void showSystemSettings();

  // Utility operations
  void resetSensorState();

private slots:
  // Unified command processor (replaces all timer tick handlers)
  void processCommandQueue();

  // Helper methods for async operations
  void completeFindOperation();
  void completeIdentifyOperation();
  void completeVerifyOperation(int id);
  void continueEnrollmentOperation(int id);

  // Polling tick handlers
  void onAuthenticationPollTick();
  void executeAdminPoll();
  void executeOperationPoll();

signals:
  // Authentication results
  void authenticationSuccess(int fingerId, int confidence);
  void authenticationFailed();
  void authenticationNoFinger();

  // Admin authentication results
  void adminFingerprintSuccess(int fingerId, int confidence);
  void adminFingerprintFailed(QString reason);
  void adminFingerprintNoFinger();

  // Enrollment signals
  void enrollmentProgress(QString message);
  void enrollmentComplete(QString message);
  void enrollmentFailed(QString error);

  // Operation result signals
  void operationComplete(QString message);
  void operationFailed(QString error);

  // Progress signals
  void progressUpdate(QString message);
  void templateCountUpdated(int count);
  void scanProgressUpdate(int percentage);

private:
  carbio::fingerprint::fingerprint_sensor
      *m_sensor; // Non-owning pointer (Controller owns it)

  // Priority-based command queue (replaces all polling timers)
  SensorCommandQueue m_commandQueue;
  QTimer *m_commandProcessorTimer; // Single timer to process command queue

  // Polling timers
  QTimer *m_authPollTimer;  // Authentication polling timer
  QTimer *m_adminPollTimer; // Admin authentication polling timer
  QTimer
      *m_operationPollTimer; // Operation polling timer (for async dialog ops)

  // Operation state tracking
  enum class OperationType : std::uint8_t {
    NONE,
    FIND,
    IDENTIFY,
    VERIFY,
    ENROLL
  };
  OperationType m_currentOperation;
  int m_operationParameter; // Used for verify (id) and enroll (id)
  int m_enrollmentStage;    // Track multi-stage enrollment progress
  std::optional<carbio::fingerprint::device_setting_info> m_cachedSettings;

  // We can't afford losing packets, hence we're using blocking queue here.
  struct EnrollmentStageResult {
    bool success;
    int stage; // 0 = first capture, 1 = second capture, 2 = model creation
    QString message;
  };
  carbio::utility::blocking_queue<EnrollmentStageResult> m_enrollmentQueue;

  // Authentication uses lockfree_triple_buffer (real-time polling results)
  struct AuthResult {
    bool valid;          // Has new data
    bool success;        // Authentication succeeded
    carbio::secure_value<int> fingerId;        // Matched finger ID (secured)
    carbio::secure_value<uint32_t> confidence; // Match confidence score (secured)
    QString message;     // Error/status message
    int scanProgress;    // Progress percentage
  };
  carbio::utility::lockfree_triple_buffer<AuthResult> m_authResultBuffer;

  // Admin authentication uses lockfree_triple_buffer
  struct AdminResult {
    bool valid;
    bool success;
    carbio::secure_value<int> fingerId;        // Matched finger ID (secured)
    carbio::secure_value<uint32_t> confidence; // Match confidence score (secured)
    QString message;
  };
  carbio::utility::lockfree_triple_buffer<AdminResult> m_adminResultBuffer;

  // Verification/Identification uses lockfree_triple_buffer (real-time updates)
  struct VerifyResult {
    bool success;
    carbio::secure_value<uint32_t> confidence; // Match confidence score (secured)
    QString message;
  };
  carbio::utility::lockfree_triple_buffer<VerifyResult> m_verifyBuffer;

  // Inline for zero-cost abstraction in hot path
  inline carbio::fingerprint::device_setting_info getDeviceSettings() {
    if (!m_cachedSettings.has_value()) [[unlikely]] {
      prewarmCache();
    }
    return m_cachedSettings.value();
  }
};
