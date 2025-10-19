import QtQuick

Canvas {
    id: progressCanvas

    // Required props
    property real progress: 0.0          // 0.0 to 1.0
    property real radius: 120
    property int lineWidth: 10
    property color progressColor: "#00DDFF"
    property color backgroundColor: "#00334B"

    // Optional
    property bool showGlow: true
    property real glowOpacity: 0.3
    property int glowLineWidth: 20

    onProgressChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()

        var centerX = width / 2
        var centerY = height / 2

        // Draw background circle
        ctx.strokeStyle = backgroundColor
        ctx.lineWidth = lineWidth
        ctx.beginPath()
        ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI, false)
        ctx.stroke()

        // Draw progress arc
        if (progress > 0) {
            ctx.strokeStyle = progressColor
            ctx.lineWidth = lineWidth
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(centerX, centerY, radius, -Math.PI / 2, -Math.PI / 2 + (progress * 2 * Math.PI), false)
            ctx.stroke()

            // Draw glow effect
            if (showGlow) {
                // Extract RGB components from QML color (0.0-1.0 range) and convert to 0-255
                ctx.strokeStyle = "rgba(" +
                    Math.round(progressColor.r * 255) + "," +
                    Math.round(progressColor.g * 255) + "," +
                    Math.round(progressColor.b * 255) + "," +
                    glowOpacity + ")"
                ctx.lineWidth = glowLineWidth
                ctx.beginPath()
                ctx.arc(centerX, centerY, radius, -Math.PI / 2, -Math.PI / 2 + (progress * 2 * Math.PI), false)
                ctx.stroke()
            }
        }
    }
}
