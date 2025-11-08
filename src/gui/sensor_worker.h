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
#include <QString>

namespace carbio
{
class fingerprint_sensor;
} // namespace carbio

class QTimer;

class SensorWorker final : public QObject
{
  Q_OBJECT

public:
  explicit SensorWorker(carbio::fingerprint_sensor* sensor, QObject* parent = nullptr);
  ~SensorWorker() override;

public slots:
  // Timer initialization (MUST be called FIRST after moveToThread!)
  void initializeTimers();

  // Sensor initialization (must run on worker thread!)
  void initializeSensor(const char* portPath);

  // Simple polling control (direct authentication polling)
  void startAuthenticationPolling(int intervalMs); // Start authentication polling at specified interval
  void stopAuthenticationPolling();                // Stop authentication polling

  // Admin authentication polling
  void startAdminPolling(int intervalMs); // Start admin auth polling
  void stopAdminPolling();                // Stop admin auth polling

  // Single-shot authentication (for dialog operations)
  void performAuthentication();
  void performAdminAuthentication();

  // Enrollment and identification
  void enrollFingerprint(int id);
  void identifyFingerprint();
  void verifyFingerprint(int id);
  void queryTemplate(int id);

  // Database operations
  void deleteFingerprint(int id);
  void clearDatabase();
  void refreshTemplateCount();
  void getEnrolledTemplateIds();

  // Notepad operations
  void writeNotepad(int pageNumber, QByteArray data);
  void readNotepad(int pageNumber);

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
  void stopAllTimers();

private slots:
  // Polling tick handlers
  void onAuthenticationPollTick();
  void executeAdminPoll();

signals:
  // Sensor initialization result
  void sensorInitialized(bool success);

  // Authentication results
  void authenticationSuccess(int fingerId, int confidence);
  void authenticationFailed();
  void authenticationNoFinger();

  // Admin authentication results
  void adminFingerprintSuccess(int fingerId, int confidence);
  void adminFingerprintFailed(const QString& reason);
  void adminFingerprintNoFinger();

  // Enrollment signals
  void enrollmentProgress(const QString& message);
  void enrollmentComplete(const QString& message);
  void enrollmentFailed(const QString& error);

  // Operation result signals
  void operationComplete(const QString& message);
  void operationFailed(const QString& error);

  // Progress signals
  void progressUpdate(const QString& message);
  void templateCountUpdated(int count);
  void scanProgressUpdate(int percentage);
  void enrolledTemplateIds(QVector<int> ids);

  // Notepad signals
  void notepadWriteComplete(int pageNumber);
  void notepadWriteFailed(int pageNumber, const QString& error);
  void notepadDataRead(int pageNumber, const QByteArray& data);
  void notepadReadFailed(int pageNumber, const QString& error);

private:
  carbio::fingerprint_sensor* m_sensor; // Non-owning pointer (Controller owns it)

  // Polling timers
  QTimer* m_authPollTimer;      // Authentication polling timer
  QTimer* m_adminPollTimer;     // Admin authentication polling timer
};
