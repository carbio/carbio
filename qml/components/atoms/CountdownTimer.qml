import QtQuick
import QtQuick.Controls

Rectangle {
    id: countdownTimer

    // Required props
    property int seconds: 0

    // Style props
    property int displayWidth: 140
    property int displayHeight: 50
    property color backgroundColor: "#2A2A2A"
    property color borderColor: "#FF3333"
    property color textColor: "#FF3333"
    property int fontSize: 32
    property bool pulseAnimation: true

    // Computed
    readonly property string formattedTime: Math.floor(seconds / 60) + ":" + (seconds % 60).toString().padStart(2, '0')

    // Geometry
    width: displayWidth
    height: displayHeight
    radius: 8

    // Appearance
    color: backgroundColor
    border.color: borderColor
    border.width: 2

    Label {
        anchors.centerIn: parent
        text: formattedTime
        font.pixelSize: fontSize
        font.family: "Courier New"
        font.bold: Font.Bold
        color: textColor

        SequentialAnimation on opacity {
            running: pulseAnimation
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 0.6; duration: 800 }
            NumberAnimation { from: 0.6; to: 1.0; duration: 800 }
        }
    }
}
