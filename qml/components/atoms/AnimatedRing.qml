import QtQuick

Rectangle {
    id: animatedRing

    // Required props
    property real ringDiameter: 240
    property int ringWidth: 4
    property color ringColor: "#0088FF"
    property real baseOpacity: 0.6

    // Animation props
    property bool animationEnabled: true
    property int pulseDuration: 1500
    property real scaleFrom: 1.0
    property real scaleTo: 1.2
    property real opacityFrom: 0.6
    property real opacityTo: 0.2

    // Geometry
    width: ringDiameter
    height: ringDiameter
    radius: ringDiameter / 2

    // Appearance
    color: "transparent"
    border.color: ringColor
    border.width: ringWidth
    opacity: baseOpacity

    // Scale animation
    SequentialAnimation on scale {
        running: animationEnabled
        loops: Animation.Infinite
        NumberAnimation { from: scaleFrom; to: scaleTo; duration: pulseDuration; easing.type: Easing.InOutQuad }
        NumberAnimation { from: scaleTo; to: scaleFrom; duration: pulseDuration; easing.type: Easing.InOutQuad }
    }

    // Opacity animation
    SequentialAnimation on opacity {
        running: animationEnabled
        loops: Animation.Infinite
        NumberAnimation { from: opacityFrom; to: opacityTo; duration: pulseDuration }
        NumberAnimation { from: opacityTo; to: opacityFrom; duration: pulseDuration }
    }
}
