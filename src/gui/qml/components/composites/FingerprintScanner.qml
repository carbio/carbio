import QtQuick
import "../atoms"
import ".."  // For ProgressRing

Item {
    id: fingerprintScanner

    // Props
    property int failedAttempts: 0
    property bool isScanning: true
    property int scanProgress: 0  // 0-100

    // Dimensions
    width: 240
    height: 300

    Column {
        anchors.centerIn: parent
        spacing: 60

        // Fingerprint scanner visualization
        Item {
            width: 240
            height: 240
            anchors.horizontalCenter: parent.horizontalCenter

            // Progress ring - shows scan progress when active
            ProgressRing {
                id: progressRing
                anchors.centerIn: parent
                implicitWidth: 240
                implicitHeight: 240
                ringWidth: 6
                ringColor: scanProgress > 0 ? "#00FF88" : "#0088FF"
                backgroundColor: "#2A2A2A"
                progress: scanProgress > 0 ? scanProgress : -2  // -2 = pulse mode
                showPercentage: false
                visible: scanProgress > 0
                opacity: scanProgress > 0 ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
            }

            // Outer pulsing ring (when not scanning) - unified ProgressRing in pulse mode
            ProgressRing {
                id: outerRing
                anchors.centerIn: parent
                implicitWidth: 240
                implicitHeight: 240
                ringWidth: 4
                ringColor: "#0088FF"
                backgroundColor: "transparent"
                progress: (isScanning && scanProgress === 0) ? -2 : 0
                showPercentage: false
                pulseScaleFrom: 1.0
                pulseScaleTo: 1.2
                pulseOpacityFrom: 0.6
                pulseOpacityTo: 0.2
                pulseDuration: 1500
                opacity: scanProgress > 0 ? 0.0 : 0.6

                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
            }

            // Middle ring - unified ProgressRing in pulse mode
            ProgressRing {
                anchors.centerIn: parent
                implicitWidth: 180
                implicitHeight: 180
                ringWidth: 3
                ringColor: "#00AAFF"
                backgroundColor: "transparent"
                progress: (isScanning && scanProgress === 0) ? -2 : 0
                showPercentage: false
                pulseScaleFrom: 1.0
                pulseScaleTo: 1.0
                pulseOpacityFrom: 0.8
                pulseOpacityTo: 0.4
                pulseDuration: 1200
                opacity: scanProgress > 0 ? 0.0 : 0.8

                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
            }

            // Fingerprint icon center
            Rectangle {
                id: fingerprintCenter
                anchors.centerIn: parent
                width: 140
                height: 140
                radius: 70
                color: "#0088FF"
                opacity: 0.2

                FingerprintRidges {
                    anchors.centerIn: parent
                    ridgeCount: 6
                    ridgeSpacing: 7
                    ridgeColor: "#FFFFFF"
                    ridgeOpacity: 0.7
                }

                SequentialAnimation on opacity {
                    running: isScanning
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.2; to: 0.45; duration: 1000 }
                    NumberAnimation { from: 0.45; to: 0.2; duration: 1000 }
                }
            }
        }

        // Status indicator dots (attempts remaining)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Repeater {
                model: 3
                StatusDot {
                    dotSize: 18
                    isFailed: index < failedAttempts
                    isActive: index === failedAttempts && isScanning
                    activeColor: "#0088FF"
                    failedColor: "#FF4444"
                    activeBorderColor: "#00AAFF"
                    failedBorderColor: "#FF6666"
                    activeOpacity: 0.4
                    failedOpacity: 0.9
                    enablePulse: false
                }
            }
        }
    }
}
