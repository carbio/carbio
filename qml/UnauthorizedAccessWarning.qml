import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: warningDialog
    anchors.fill: parent
    color: "#EE000000"
    visible: false
    z: 600

    property string warningMessage: ""

    signal acknowledged()

    function show(message) {
        warningMessage = message
        visible = true
        openAnimation.start()
        pulseAnimation.start()

        // Auto-close after 10 seconds
        autoCloseTimer.restart()
    }

    function close() {
        closeAnimation.start()
        pulseAnimation.stop()
        autoCloseTimer.stop()
    }

    Timer {
        id: autoCloseTimer
        interval: 10000
        running: false
        repeat: false
        onTriggered: close()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {} // Prevent click-through
    }

    Rectangle {
        id: dialogBox
        anchors.centerIn: parent
        width: 600
        height: 450
        color: "#2A2A2A"
        radius: 20
        border.color: "#FF3333"
        border.width: 4
        scale: 0.8
        opacity: 0

        SequentialAnimation {
            id: pulseAnimation
            running: false
            loops: Animation.Infinite

            NumberAnimation {
                target: dialogBox.border
                property: "width"
                from: 4
                to: 6
                duration: 800
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: dialogBox.border
                property: "width"
                from: 6
                to: 4
                duration: 800
                easing.type: Easing.InOutQuad
            }
        }

        NumberAnimation {
            id: openAnimation
            target: dialogBox
            properties: "scale,opacity"
            to: 1.0
            duration: 400
            easing.type: Easing.OutBack
        }

        NumberAnimation {
            id: closeAnimation
            target: dialogBox
            properties: "scale,opacity"
            to: 0.8
            duration: 250
            easing.type: Easing.InQuad
            onFinished: {
                warningDialog.opacity = 0
                warningDialog.visible = false
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 35
            spacing: 25


            // Title
            Label {
                Layout.fillWidth: true
                text: "UNAUTHORIZED ACCESS"
                font.pixelSize: 28
                font.family: "Inter"
                font.bold: Font.ExtraBold
                color: "#FF3333"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            // Message box
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#1E1E1E"
                radius: 12
                border.color: "#FF6666"
                border.width: 2

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 5
                    clip: true

                    Label {
                        width: parent.width
                        text: warningMessage
                        font.pixelSize: 16
                        font.family: "Inter"
                        color: "#FFFFFF"
                        wrapMode: Text.WordWrap
                        lineHeight: 1.4
                    }
                }
            }

            // Info text
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "#3A3A3A"
                radius: 10
                border.color: "#555555"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 3

                    Label {
                        Layout.fillWidth: true
                        text: "📋 This incident has been recorded:"
                        font.pixelSize: 14
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: "#FFAA00"
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• Encrypted audit log entry created\n• Timestamp: " + new Date().toLocaleString()
                        font.pixelSize: 12
                        font.family: "Inter"
                        color: "#CCCCCC"
                        wrapMode: Text.WordWrap
                        lineHeight: 1.3
                    }
                }
            }

            // Acknowledge button
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 55
                color: ackMouseArea.containsMouse ? "#FF5555" : "#FF3333"
                radius: 12
                border.color: "#FF6666"
                border.width: 2

                scale: ackMouseArea.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: ackMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        close()
                        acknowledged()
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: "I Understand"
                    font.pixelSize: 18
                    font.family: "Inter"
                    font.bold: Font.Bold
                    color: "#FFFFFF"
                }
            }
        }
    }
}
