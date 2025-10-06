import QtQuick
import "../atoms"

Item {
    id: lockoutDisplay

    // Props
    property int lockoutSeconds: 0
    property int maxLockoutSeconds: 20
    property bool isLocked: true

    // Dimensions
    width: 280
    height: 400

    // Readonly computed
    readonly property real countdownProgress: maxLockoutSeconds > 0 ? lockoutSeconds / maxLockoutSeconds : 0

    Column {
        anchors.centerIn: parent
        spacing: 50

        // Lockout indicator with countdown
        Item {
            width: 280
            height: 280
            anchors.horizontalCenter: parent.horizontalCenter

            // Pulsing danger circle
            Rectangle {
                anchors.centerIn: parent
                width: 260
                height: 260
                radius: 130
                color: "#FF3333"
                opacity: 0.1

                SequentialAnimation on scale {
                    running: isLocked
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.15; duration: 1000; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.15; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
                }

                SequentialAnimation on opacity {
                    running: isLocked
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.1; to: 0.25; duration: 1000 }
                    NumberAnimation { from: 0.25; to: 0.1; duration: 1000 }
                }
            }

            // Countdown circle progress
            ProgressCanvas {
                id: countdownCircle
                anchors.fill: parent
                progress: countdownProgress
                radius: 120
                lineWidth: 12
                progressColor: "#FF3333"
                backgroundColor: "#442222"
                showGlow: false
            }

            // Lock icon in center
            LockIcon {
                anchors.centerIn: parent
                lockColor: "#FF3333"
                lockOpacity: 0.8
            }

            // Time remaining display
            CountdownTimer {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                seconds: lockoutSeconds
                displayWidth: 140
                displayHeight: 50
                backgroundColor: "#2A2A2A"
                borderColor: "#FF3333"
                textColor: "#FF3333"
                fontSize: 32
                pulseAnimation: isLocked
            }
        }

        // Failed attempt indicators (all filled)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Repeater {
                model: 3
                Item {
                    width: 28
                    height: 28

                    StatusDot {
                        anchors.fill: parent
                        dotSize: 28
                        isFailed: true
                        isActive: false
                        failedColor: "#FF3333"
                        failedBorderColor: "#FF6666"
                        failedOpacity: 0.9
                        enablePulse: false
                    }

                    SequentialAnimation on opacity {
                        running: isLocked
                        loops: Animation.Infinite
                        PauseAnimation { duration: index * 200 }
                        NumberAnimation { from: 0.9; to: 0.4; duration: 600 }
                        NumberAnimation { from: 0.4; to: 0.9; duration: 600 }
                    }

                    // X mark
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.strokeStyle = "#FFFFFF"
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"

                            var margin = 6
                            var size = parent.width - margin * 2

                            ctx.beginPath()
                            ctx.moveTo(margin, margin)
                            ctx.lineTo(margin + size, margin + size)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(margin + size, margin)
                            ctx.lineTo(margin, margin + size)
                            ctx.stroke()
                        }
                    }
                }
            }
        }
    }
}
