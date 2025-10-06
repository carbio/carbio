import QtQuick

Canvas {
    id: xmarkCanvas

    // Required props
    property real progress: 0.0          // 0.0 to 1.0 (animated)

    // Style props
    property color strokeColor: "#FF3333"
    property int strokeWidth: 14

    // Fixed size
    width: 160
    height: 160

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

        var margin = 30
        var size = 100

        // Draw first diagonal (top-left to bottom-right)
        if (progress > 0) {
            ctx.beginPath()
            ctx.moveTo(margin, margin)
            var p1 = Math.min(progress * 2, 1)
            ctx.lineTo(margin + size * p1, margin + size * p1)
            ctx.stroke()
        }

        // Draw second diagonal (top-right to bottom-left)
        if (progress > 0.5) {
            ctx.beginPath()
            ctx.moveTo(margin + size, margin)
            var p2 = Math.min((progress - 0.5) * 2, 1)
            ctx.lineTo(margin + size - size * p2, margin + size * p2)
            ctx.stroke()
        }
    }
}
