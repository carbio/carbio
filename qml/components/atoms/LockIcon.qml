import QtQuick

Item {
    id: lockIcon

    // Required props
    property color lockColor: "#FF3333"
    property real lockOpacity: 0.8

    // Dimensions (fixed for proper proportions)
    width: 100
    height: 120

    // Lock body
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: 80
        height: 60
        radius: 8
        color: lockColor
        opacity: lockOpacity

        // Keyhole
        Rectangle {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 5
            width: 12
            height: 12
            radius: 6
            color: "#1E1E1E"
        }

        Rectangle {
            anchors.top: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 8
            height: 16
            color: "#1E1E1E"
        }
    }

    // Lock shackle
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 45
        anchors.horizontalCenter: parent.horizontalCenter
        width: 60
        height: 50
        radius: 30
        color: "transparent"
        border.color: lockColor
        border.width: 10
        clip: true

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.height / 2
            color: "#1E1E1E"
        }
    }
}
