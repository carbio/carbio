import QtQuick

Column {
    id: fingerprintRidges

    // Props
    property int ridgeCount: 6
    property int ridgeSpacing: 7
    property color ridgeColor: "#FFFFFF"
    property real ridgeOpacity: 0.7

    spacing: ridgeSpacing

    Repeater {
        model: ridgeCount
        Rectangle {
            width: 70 - (index * 10)
            height: 4
            radius: 2
            color: ridgeColor
            opacity: ridgeOpacity
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
