import QtQuick
import QtQuick.Controls
import "../atoms"

Item {
    id: authProgress

    // Props
    property bool isProcessing: true
    property int scanProgress: 0  // 0-100 from controller

    // Dimensions
    width: 280
    height: 400

    // Readonly computed - convert 0-100 to 0.0-1.0
    readonly property real totalProgress: scanProgress / 100.0
    readonly property int currentStage: {
        if (scanProgress < 33) return 0      // Capturing
        else if (scanProgress < 66) return 1 // Processing
        else return 2                         // Verifying
    }
    readonly property real stageProgress: {
        if (scanProgress < 33) return (scanProgress / 33.0)
        else if (scanProgress < 66) return ((scanProgress - 33) / 33.0)
        else return ((scanProgress - 66) / 34.0)
    }

    onIsProcessingChanged: {
        if (!isProcessing) {
            // Progress will be reset by controller
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 50

        Item {
            width: 280
            height: 280
            anchors.horizontalCenter: parent.horizontalCenter

            // Background circle
            Rectangle {
                anchors.centerIn: parent
                width: 260
                height: 260
                radius: 130
                color: "transparent"
                border.color: "#00334B"
                border.width: 8
            }

            // Progress ring showing overall completion
            ProgressCanvas {
                id: progressRing
                anchors.fill: parent
                progress: totalProgress
                radius: 120
                lineWidth: 10
                progressColor: "#00DDFF"
                backgroundColor: "#00334B"
                showGlow: true
                glowOpacity: 0.3
                glowLineWidth: 20
            }

            // Center visualization showing current stage
            Item {
                anchors.centerIn: parent
                width: 180
                height: 180

                // Stage 0: Capturing fingerprint image
                Rectangle {
                    anchors.centerIn: parent
                    width: 160
                    height: 160
                    radius: 80
                    color: "#00DDFF"
                    opacity: currentStage === 0 ? 0.3 : 0.1
                    visible: currentStage === 0

                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    // Fingerprint being captured
                    Column {
                        anchors.centerIn: parent
                        spacing: 6
                        Repeater {
                            model: 6
                            Rectangle {
                                width: 80 - (index * 12)
                                height: 4
                                radius: 2
                                color: "#FFFFFF"
                                opacity: index < (stageProgress * 6) ? 1.0 : 0.3
                                anchors.horizontalCenter: parent.horizontalCenter
                                Behavior on opacity { NumberAnimation { duration: 200 } }
                            }
                        }
                    }
                }

                // Stage 1: Creating template
                Rectangle {
                    anchors.centerIn: parent
                    width: 160
                    height: 160
                    radius: 80
                    color: "#00DDFF"
                    opacity: currentStage === 1 ? 0.3 : 0.1
                    visible: currentStage === 1

                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    // Data processing visualization
                    Grid {
                        anchors.centerIn: parent
                        columns: 4
                        spacing: 8

                        Repeater {
                            model: 16
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 3
                                color: "#FFFFFF"
                                opacity: index < (stageProgress * 16) ? 0.9 : 0.2
                                Behavior on opacity { NumberAnimation { duration: 150 } }
                            }
                        }
                    }
                }

                // Stage 2: Searching database
                Rectangle {
                    anchors.centerIn: parent
                    width: 160
                    height: 160
                    radius: 80
                    color: "#00DDFF"
                    opacity: currentStage === 2 ? 0.3 : 0.1
                    visible: currentStage === 2

                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    // Search radar effect
                    Canvas {
                        anchors.fill: parent
                        property real searchAngle: stageProgress * 360

                        onSearchAngleChanged: requestPaint()

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()

                            var centerX = width / 2
                            var centerY = height / 2

                            // Radar sweep line
                            ctx.strokeStyle = "#FFFFFF"
                            ctx.lineWidth = 3
                            ctx.beginPath()
                            ctx.moveTo(centerX, centerY)
                            var angle = (searchAngle - 90) * Math.PI / 180
                            ctx.lineTo(centerX + Math.cos(angle) * 70, centerY + Math.sin(angle) * 70)
                            ctx.stroke()
                        }
                    }

                    // Database scan rings
                    Repeater {
                        model: 3
                        Rectangle {
                            anchors.centerIn: parent
                            width: 40 + (index * 30)
                            height: 40 + (index * 30)
                            radius: (40 + (index * 30)) / 2
                            color: "transparent"
                            border.color: "#FFFFFF"
                            border.width: 1
                            opacity: 0.4
                        }
                    }
                }
            }

            // Percentage indicator
            Label {
                anchors.top: parent.bottom
                anchors.topMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.floor(totalProgress * 100) + "%"
                font.pixelSize: 28
                font.bold: Font.Bold
                font.family: "Courier New"
                color: "#00DDFF"
                opacity: 0.8
            }
        }

        // Stage indicator dots
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            Repeater {
                model: 3
                StatusDot {
                    dotSize: 12
                    isFailed: false
                    isActive: index === currentStage
                    activeColor: "#00DDFF"
                    failedColor: "#003344"
                    activeBorderColor: "#00DDFF"
                    failedBorderColor: "#00DDFF"
                    activeOpacity: index <= currentStage ? 1.0 : 0.4
                    failedOpacity: 0.4
                    enablePulse: index === currentStage
                    pulseDuration: 400
                }
            }
        }
    }
}
