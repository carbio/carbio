import QtQuick

Canvas {
    id: checkmarkCanvas

    // Required props
    property real progress: 0.0          // 0.0 to 1.0 (animated)

    // Style props
    property color strokeColor: "#32D74B"
    property int strokeWidth: 12

    // Fixed size
    width: 180
    height: 180

    // Signal
    signal drawingComplete()

    onProgressChanged: {
        requestPaint()
        if (progress >= 1.0) {
            drawingComplete()
        }
    }

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()

        ctx.strokeStyle = strokeColor
        ctx.lineWidth = strokeWidth
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        // Checkmark path
        var startX = 40
        var startY = 90
        var midX = 75
        var midY = 130
        var endX = 140
        var endY = 50

        // Draw first segment (down stroke)
        if (progress > 0) {
            ctx.beginPath()
            ctx.moveTo(startX, startY)
            var p1 = Math.min(progress * 2, 1)
            ctx.lineTo(startX + (midX - startX) * p1, startY + (midY - startY) * p1)
            ctx.stroke()
        }

        // Draw second segment (up stroke)
        if (progress > 0.5) {
            ctx.beginPath()
            ctx.moveTo(midX, midY)
            var p2 = Math.min((progress - 0.5) * 2, 1)
            ctx.lineTo(midX + (endX - midX) * p2, midY + (endY - midY) * p2)
            ctx.stroke()
        }
    }
}
