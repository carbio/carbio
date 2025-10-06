import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: infoDialog
    modal: true
    anchors.centerIn: parent
    width: 900
    height: 600

    property string infoTitle: "Information"
    property string infoMessage: ""
    property bool isError: false

    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: isError ? "#FF3333" : "#01E4E0"
        border.width: 2
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20

        // Header with icon and title
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            Rectangle {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                radius: 25
                color: isError ? "#FF3333" : "#01E4E0"
                opacity: 0.2

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = infoDialog.isError ? "#FF3333" : "#01E4E0"
                        ctx.lineWidth = 3
                        ctx.lineCap = "round"

                        if (infoDialog.isError) {
                            // Draw X
                            var margin = 12
                            var size = 26
                            ctx.beginPath()
                            ctx.moveTo(margin, margin)
                            ctx.lineTo(margin + size, margin + size)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.moveTo(margin + size, margin)
                            ctx.lineTo(margin, margin + size)
                            ctx.stroke()
                        } else {
                            // Draw info icon (i)
                            ctx.fillStyle = infoDialog.isError ? "#FF3333" : "#01E4E0"
                            ctx.beginPath()
                            ctx.arc(25, 18, 3, 0, 2 * Math.PI)
                            ctx.fill()

                            ctx.lineWidth = 4
                            ctx.beginPath()
                            ctx.moveTo(25, 25)
                            ctx.lineTo(25, 38)
                            ctx.stroke()
                        }
                    }

                    Component.onCompleted: requestPaint()
                }
            }

            Label {
                text: infoTitle
                font.pixelSize: 24
                font.family: "Inter"
                font.bold: Font.Bold
                color: isError ? "#FF3333" : "#01E4E0"
                Layout.fillWidth: true
            }

            // Close button
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: 20
                color: closeMouseArea.containsMouse ? "#4A4A4A" : "#3A3A3A"
                border.color: "#666666"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: closeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: infoDialog.close()
                }

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.strokeStyle = "#CCCCCC"
                        ctx.lineWidth = 2
                        ctx.lineCap = "round"

                        var margin = 12
                        var size = 16

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

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444444"
        }

        // Scrollable content area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Label {
                id: messageLabel
                width: parent.width
                text: infoMessage
                font.pixelSize: 18
                font.family: "Courier New"
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }
        }

        // OK button at bottom
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: "OK"

            contentItem: Label {
                text: parent.text
                font.pixelSize: 18
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#1E1E1E"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: {
                    if (parent.down) return "#00B8B0"
                    if (parent.hovered) return "#00FFEE"
                    return "#01E4E0"
                }
                radius: 8
                border.color: "#00FFEE"
                border.width: 2

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            onClicked: infoDialog.close()
        }
    }

    function show(title, message, error) {
        infoTitle = title
        infoMessage = message
        isError = error || false
        open()
    }

    enter: Transition {
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            duration: 250
            easing.type: Easing.OutBack
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            duration: 150
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 150
        }
    }
}