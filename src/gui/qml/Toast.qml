import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: toast

    // Safer parent references - cache parent dimensions to avoid crashes
    property real parentWidth: parent ? parent.width : 800
    property real parentHeight: parent ? parent.height : 600

    x: (parentWidth - width) / 2
    y: parentHeight - height - 100
    width: Math.min(600, parentWidth * 0.8)
    height: contentColumn.implicitHeight + 40
    modal: false  // Toast doesn't block interaction
    focus: false
    closePolicy: Popup.NoAutoClose

    property string message: ""
    property bool isError: false
    property int duration: 3000

    // No dimmed overlay for toast
    Overlay.modal: Item {}
    Overlay.modeless: Item {}

    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.width: 2
        border.color: toast.isError ? "#FF3333" : "#32D74B"

        // Drop shadow effect (Qt 6 compatible)
        layer.enabled: true
        layer.smooth: true
    }
    
    // Open animation
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            duration: 200
            easing.type: Easing.OutBack
        }
    }
    
    // Close animation
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 250
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.8
            duration: 250
        }
    }
    
    contentItem: ColumnLayout {
        id: contentColumn
        spacing: 0
        
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20
            spacing: 15
            
            // Icon
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: 20
                color: toast.isError ? "#FF3333" : "#32D74B"
                opacity: 0.2
                
                Canvas {
                    id: iconCanvas
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = toast.isError ? "#FF3333" : "#32D74B"
                        ctx.lineWidth = 3
                        ctx.lineCap = "round"

                        if (toast.isError) {
                            // Draw X
                            var margin = 10
                            var size = 20
                            ctx.beginPath()
                            ctx.moveTo(margin, margin)
                            ctx.lineTo(margin + size, margin + size)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.moveTo(margin + size, margin)
                            ctx.lineTo(margin, margin + size)
                            ctx.stroke()
                        } else {
                            // Draw checkmark
                            ctx.beginPath()
                            ctx.moveTo(10, 20)
                            ctx.lineTo(16, 26)
                            ctx.lineTo(30, 12)
                            ctx.stroke()
                        }
                    }

                    Component.onCompleted: requestPaint()

                    Connections {
                        target: toast
                        function onIsErrorChanged() {
                            iconCanvas.requestPaint()
                        }
                    }
                }
            }
            
            // Message text
            Label {
                Layout.fillWidth: true
                text: toast.message
                font.pixelSize: 16
                font.family: "Inter"
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
                lineHeight: 1.3
            }
            
            // Close button
            Button {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                
                background: Rectangle {
                    radius: 15
                    color: parent.hovered ? "#4A4A4A" : "#3A3A3A"
                    border.color: "#666666"
                    border.width: 1
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                
                contentItem: Canvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.strokeStyle = "#CCCCCC"
                        ctx.lineWidth = 2
                        ctx.lineCap = "round"
                        
                        var margin = 9
                        var size = 12
                        
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
                
                onClicked: toast.close()
            }
        }
    }
    
    Timer {
        id: hideTimer
        interval: toast.duration
        onTriggered: toast.close()
    }
    
    function show(msg, error) {
        // Close first if already open to avoid re-entrancy issues
        if (opened) {
            close()
        }

        message = msg
        isError = error || false

        // Open immediately instead of deferred - the cached parent dimensions
        // make it safe to open even during complex signal chains
        open()
        hideTimer.restart()
    }
}