import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: toast
    width: Math.min(parent.width * 0.8, 600)
    height: contentColumn.height + 40
    radius: 12
    color: "#2A2A2A"
    border.width: 2
    border.color: isError ? "#FF3333" : "#32D74B"
    opacity: 0
    visible: false
    z: 10000

    property string message: ""
    property bool isError: false
    property int duration: 3000

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 100

    scale: 0.8

    Behavior on opacity { NumberAnimation { duration: 200 } }
    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            // Icon
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: 20
                color: isError ? "#FF3333" : "#32D74B"
                opacity: 0.2

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = toast.isError ? "#FF3333" : "#32D74B"
                        ctx.lineWidth = 3
                        ctx.lineCap = "round"

                        if (toast.isError) {
                            // Draw X
                            var margin = 10
                            var size = 20
                            ctx.beginPath()
                            ctx.moveTo(margin, margin)
                            ctx.lineTo(margin + size, margin + size)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.moveTo(margin + size, margin)
                            ctx.lineTo(margin, margin + size)
                            ctx.stroke()
                        } else {
                            // Draw checkmark
                            ctx.beginPath()
                            ctx.moveTo(10, 20)
                            ctx.lineTo(16, 26)
                            ctx.lineTo(30, 12)
                            ctx.stroke()
                        }
                    }

                    Component.onCompleted: requestPaint()
                }
            }

            // Message text
            Label {
                Layout.fillWidth: true
                text: toast.message
                font.pixelSize: 16
                font.family: "Inter"
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
                lineHeight: 1.3
            }

            // Close button
            Rectangle {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                radius: 15
                color: closeMouseArea.containsMouse ? "#4A4A4A" : "#3A3A3A"
                border.color: "#666666"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: closeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: toast.hide()
                }

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.strokeStyle = "#CCCCCC"
                        ctx.lineWidth = 2
                        ctx.lineCap = "round"

                        var margin = 9
                        var size = 12

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

    Timer {
        id: hideTimer
        interval: toast.duration
        onTriggered: toast.hide()
    }

    function show(msg, error) {
        message = msg
        isError = error || false
        visible = true
        opacity = 1
        scale = 1.0
        hideTimer.restart()
    }

    function hide() {
        opacity = 0
        scale = 0.8
        hideTimer.stop()
        hideDelayTimer.start()
    }

    Timer {
        id: hideDelayTimer
        interval: 250
        onTriggered: toast.visible = false
    }
}
