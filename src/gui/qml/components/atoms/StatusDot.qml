import QtQuick

Rectangle {
    id: statusDot

    // Required props
    property bool isFailed: false
    property bool isActive: false

    // Style props
    property int dotSize: 18
    property color activeColor: "#0088FF"
    property color failedColor: "#FF4444"
    property color activeBorderColor: "#00AAFF"
    property color failedBorderColor: "#FF6666"
    property real activeOpacity: 0.4
    property real failedOpacity: 0.9

    // Animation
    property bool enablePulse: false
    property int pulseDuration: 400

    // Geometry
    width: dotSize
    height: dotSize
    radius: dotSize / 2

    // Appearance
    color: isFailed ? failedColor : activeColor
    opacity: isFailed ? failedOpacity : activeOpacity
    border.color: isFailed ? failedBorderColor : activeBorderColor
    border.width: isFailed ? 2 : 1

    // Smooth transitions
    Behavior on color { ColorAnimation { duration: 300 } }
    Behavior on opacity { NumberAnimation { duration: 300 } }

    // Pulse animation when enabled
    SequentialAnimation on scale {
        running: enablePulse && isActive
        loops: Animation.Infinite
        NumberAnimation { from: 1.0; to: 1.4; duration: pulseDuration }
        NumberAnimation { from: 1.4; to: 1.0; duration: pulseDuration }
    }
}
