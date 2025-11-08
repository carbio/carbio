import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // Public properties
    property int progress: 0                    // 0-100 for determinate, -1 for indeterminate, -2 for pulse-only mode
    property color ringColor: "#01E4E0"
    property color backgroundColor: "#2A2A2A"
    property int ringWidth: 8
    property string centerText: ""
    property int centerTextSize: 24
    property color centerTextColor: "#FFFFFF"
    property bool showPercentage: true

    // Pulse animation properties (for AnimatedRing replacement)
    property bool pulseEnabled: progress === -2
    property int pulseDuration: 1500
    property real pulseScaleFrom: 1.0
    property real pulseScaleTo: 1.2
    property real pulseOpacityFrom: 0.6
    property real pulseOpacityTo: 0.2

    implicitWidth: 180
    implicitHeight: 180

    // Background circle
    Canvas {
        id: backgroundCircle
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var centerX = width / 2
            var centerY = height / 2
            var radius = Math.min(width, height) / 2 - root.ringWidth / 2

            ctx.strokeStyle = root.backgroundColor
            ctx.lineWidth = root.ringWidth
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI)
            ctx.stroke()
        }
    }

    // Progress arc
    Canvas {
        id: progressArc
        anchors.fill: parent
        rotation: -90

        property real animatedProgress: 0

        Behavior on animatedProgress {
            NumberAnimation {
                duration: 400
                easing.type: Easing.OutCubic
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var centerX = width / 2
            var centerY = height / 2
            var radius = Math.min(width, height) / 2 - root.ringWidth / 2

            var startAngle = 0
            var endAngle = (animatedProgress / 100) * 2 * Math.PI

            // Create gradient
            var gradient = ctx.createLinearGradient(0, 0, width, height)
            gradient.addColorStop(0, root.ringColor)
            gradient.addColorStop(1, Qt.lighter(root.ringColor, 1.3))

            ctx.strokeStyle = gradient
            ctx.lineWidth = root.ringWidth
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius, startAngle, endAngle)
            ctx.stroke()
        }

        onAnimatedProgressChanged: requestPaint()
    }

    // Indeterminate spinner overlay
    Canvas {
        id: spinnerArc
        anchors.fill: parent
        visible: root.progress < 0
        rotation: spinnerRotation

        property real spinnerRotation: 0

        RotationAnimation on spinnerRotation {
            running: root.progress < 0
            from: 0
            to: 360
            duration: 1200
            loops: Animation.Infinite
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var centerX = width / 2
            var centerY = height / 2
            var radius = Math.min(width, height) / 2 - root.ringWidth / 2

            var startAngle = 0
            var endAngle = 0.75 * Math.PI

            // Create gradient
            var gradient = ctx.createLinearGradient(0, 0, width, height)
            gradient.addColorStop(0, root.ringColor)
            gradient.addColorStop(1, Qt.lighter(root.ringColor, 1.5))

            ctx.strokeStyle = gradient
            ctx.lineWidth = root.ringWidth
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius, startAngle, endAngle)
            ctx.stroke()
        }
    }

    // Center content
    Item {
        anchors.centerIn: parent
        width: parent.width * 0.6
        height: parent.height * 0.6

        Column {
            anchors.centerIn: parent
            spacing: 5

            // Custom center text
            Label {
                visible: root.centerText !== ""
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.centerText
                font.pixelSize: root.centerTextSize
                font.family: "Inter"
                font.bold: Font.Bold
                color: root.centerTextColor
                horizontalAlignment: Text.AlignHCenter
            }

            // Percentage text (shown if no custom text and determinate progress)
            Label {
                visible: root.centerText === "" && root.progress >= 0 && root.showPercentage
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(progressArc.animatedProgress) + "%"
                font.pixelSize: root.centerTextSize
                font.family: "Courier New"
                font.bold: Font.Bold
                color: root.centerTextColor
            }

            // Pulsing dot for indeterminate
            Rectangle {
                visible: root.progress < 0 && root.centerText === ""
                anchors.horizontalCenter: parent.horizontalCenter
                width: 12
                height: 12
                radius: 6
                color: root.ringColor

                SequentialAnimation on opacity {
                    running: root.progress < 0
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.3; to: 1.0; duration: 600 }
                    NumberAnimation { from: 1.0; to: 0.3; duration: 600 }
                }
            }
        }
    }

    // Glow effect when processing
    Rectangle {
        anchors.centerIn: parent
        width: parent.width + 20
        height: parent.height + 20
        radius: width / 2
        color: "transparent"
        border.color: root.ringColor
        border.width: 2
        opacity: 0
        visible: root.progress >= 0 || root.progress === -1

        SequentialAnimation on opacity {
            running: root.progress >= 0 || root.progress === -1
            loops: Animation.Infinite
            NumberAnimation { from: 0; to: 0.3; duration: 800 }
            NumberAnimation { from: 0.3; to: 0; duration: 800 }
        }

        SequentialAnimation on scale {
            running: root.progress >= 0 || root.progress === -1
            loops: Animation.Infinite
            NumberAnimation { from: 0.95; to: 1.05; duration: 800 }
            NumberAnimation { from: 1.05; to: 0.95; duration: 800 }
        }
    }

    // Pulse animations (for AnimatedRing mode)
    SequentialAnimation on scale {
        running: root.pulseEnabled
        loops: Animation.Infinite
        NumberAnimation { from: root.pulseScaleFrom; to: root.pulseScaleTo; duration: root.pulseDuration; easing.type: Easing.InOutQuad }
        NumberAnimation { from: root.pulseScaleTo; to: root.pulseScaleFrom; duration: root.pulseDuration; easing.type: Easing.InOutQuad }
    }

    SequentialAnimation on opacity {
        running: root.pulseEnabled
        loops: Animation.Infinite
        NumberAnimation { from: root.pulseOpacityFrom; to: root.pulseOpacityTo; duration: root.pulseDuration }
        NumberAnimation { from: root.pulseOpacityTo; to: root.pulseOpacityFrom; duration: root.pulseDuration }
    }

    // Update progress
    onProgressChanged: {
        if (progress >= 0 && progress <= 100) {
            progressArc.animatedProgress = progress
        }
        backgroundCircle.requestPaint()
        progressArc.requestPaint()
        spinnerArc.requestPaint()
    }

    Component.onCompleted: {
        backgroundCircle.requestPaint()
        progressArc.requestPaint()
        spinnerArc.requestPaint()
    }
}
