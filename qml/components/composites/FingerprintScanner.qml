import QtQuick
import "../atoms"

Item {
    id: fingerprintScanner

    // Props
    property int failedAttempts: 0
    property bool isScanning: true

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

            // Outer pulsing ring
            AnimatedRing {
                id: outerRing
                anchors.centerIn: parent
                ringDiameter: 240
                ringWidth: 4
                ringColor: "#0088FF"
                baseOpacity: 0.6
                animationEnabled: isScanning
                pulseDuration: 1500
                scaleFrom: 1.0
                scaleTo: 1.2
                opacityFrom: 0.6
                opacityTo: 0.2
            }

            // Middle ring
            AnimatedRing {
                anchors.centerIn: parent
                ringDiameter: 180
                ringWidth: 3
                ringColor: "#00AAFF"
                baseOpacity: 0.8
                animationEnabled: isScanning
                pulseDuration: 1200
                scaleFrom: 1.0
                scaleTo: 1.0
                opacityFrom: 0.8
                opacityTo: 0.4
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
