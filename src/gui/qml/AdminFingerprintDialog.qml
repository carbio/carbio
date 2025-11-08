import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components/composites"

Popup {
    id: adminFingerprintDialog
    anchors.centerIn: parent
    width: 600
    height: 700
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose  // Prevent closing during scan

    property int scanProgress: 0  // Bind to controller.scanProgress

    // Dim background
    Overlay.modal: Rectangle {
        color: "#DD000000"
    }

    // Custom background with border and styling
    background: Rectangle {
        id: dialogBox
        color: "#2A2A2A"
        radius: 20
        border.color: "#01E4E0"
        border.width: 3

        // Open animation
        scale: adminFingerprintDialog.opened ? 1.0 : 0.8
        Behavior on scale {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutBack
            }
        }
    }

    onAboutToShow: {
        scanProgress = 0
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25

        // Header
        Label {
            text: "Admin Biometric Verification"
            font.pixelSize: 24
            font.family: "Inter"
            font.bold: Font.Bold
            color: "#01E4E0"
            Layout.alignment: Qt.AlignHCenter
        }

        // Step indicator
        Label {
            text: "Step 2 of 2: Fingerprint Authentication"
            font.pixelSize: 16
            font.family: "Inter"
            color: "#AAAAAA"
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444444"
        }

        // Real-time fingerprint scanner with progress
        FingerprintScanner {
            id: fingerprintScanner
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 240
            Layout.preferredHeight: 300
            failedAttempts: 0
            isScanning: adminFingerprintDialog.visible
            scanProgress: adminFingerprintDialog.scanProgress
        }

        // Status label
        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: {
                if (scanProgress === 0) return "Place your admin fingerprint on the sensor..."
                else if (scanProgress < 33) return "Capturing fingerprint image..."
                else if (scanProgress < 66) return "Processing fingerprint..."
                else if (scanProgress < 100) return "Verifying against database..."
                else return "Complete!"
            }
            font.pixelSize: 15
            font.family: "Inter"
            color: scanProgress > 0 ? "#01E4E0" : "#FFFFFF"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            Behavior on color { ColorAnimation { duration: 200 } }
        }

        // Progress percentage
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: scanProgress > 0 ? scanProgress + "%" : ""
            font.pixelSize: 28
            font.family: "Courier New"
            font.bold: Font.Bold
            color: "#01E4E0"
            opacity: scanProgress > 0 ? 1.0 : 0.0

            Behavior on opacity { NumberAnimation { duration: 200 } }
        }

        Item { Layout.fillHeight: true }

        // Info text
        Label {
            Layout.fillWidth: true
            text: "Only admin fingerprints (IDs 0-2) can access this menu"
            font.pixelSize: 12
            font.family: "Inter"
            color: "#888888"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            opacity: 0.7
        }
    }
}
