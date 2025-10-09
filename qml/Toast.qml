import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: toast
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - 100 : 100
    width: Math.min(600, parent ? parent.width * 0.8 : 600)
    height: contentColumn.height + 40
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
        
        // Popup handles opacity animation
        layer.enabled: true
        layer.effect: ShaderEffect {
            property real shadow: 0.5
            fragmentShader: "
                varying highp vec2 qt_TexCoord0;
                uniform sampler2D source;
                uniform lowp float qt_Opacity;
                uniform lowp float shadow;
                void main() {
                    gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity;
                }
            "
        }
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
                            parent.requestPaint()
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
        message = msg
        isError = error || false
        open()
        hideTimer.restart()
    }
}