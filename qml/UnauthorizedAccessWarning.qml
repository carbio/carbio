import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: warningDialog
    anchors.centerIn: parent
    width: 600
    height: 450
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    
    property string warningMessage: ""
    
    signal acknowledged()
    
    Overlay.modal: Rectangle {
        color: "#EE000000"
    }
    
    background: Rectangle {
        id: dialogBox
        color: "#2A2A2A"
        radius: 20
        border.color: "#FF3333"
        border.width: 4
        
        // Pulse animation
        SequentialAnimation {
            running: warningDialog.opened
            loops: Animation.Infinite
            
            NumberAnimation {
                target: dialogBox.border
                property: "width"
                from: 4
                to: 6
                duration: 800
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: dialogBox.border
                property: "width"
                from: 6
                to: 4
                duration: 800
                easing.type: Easing.InOutQuad
            }
        }
        
        // Open animation
        scale: warningDialog.opened ? 1.0 : 0.8
        Behavior on scale {
            NumberAnimation {
                duration: 400
                easing.type: Easing.OutBack
            }
        }
    }
    
    function show(message) {
        warningMessage = message
        open()
    }
    
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 35
        spacing: 25
        Canvas {
            Layout.alignment: Qt.AlignHCenter
            width: 80
            height: 80
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                
                ctx.strokeStyle = "#FF3333"
                ctx.fillStyle = "#FF3333"
                ctx.lineWidth = 3
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                
                ctx.beginPath()
                ctx.moveTo(40, 10)
                ctx.lineTo(65, 20)
                ctx.lineTo(65, 45)
                ctx.quadraticCurveTo(65, 60, 40, 70)
                ctx.quadraticCurveTo(15, 60, 15, 45)
                ctx.lineTo(15, 20)
                ctx.closePath()
                ctx.stroke()
                
                ctx.fillStyle = "#FF3333"
                ctx.fillRect(37, 28, 6, 20)
                ctx.beginPath()
                ctx.arc(40, 54, 3, 0, 2 * Math.PI)
                ctx.fill()
            }
            
            Component.onCompleted: requestPaint()
        }
        
        // Title
        Label {
            Layout.fillWidth: true
            text: "UNAUTHORIZED ACCESS"
            font.pixelSize: 28
            font.family: "Inter"
            font.bold: Font.ExtraBold
            color: "#FF3333"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
        
        // Message box
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#1E1E1E"
            radius: 12
            border.color: "#FF6666"
            border.width: 2
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 15
                clip: true
                
                Label {
                    width: parent.width
                    text: warningMessage
                    font.pixelSize: 16
                    font.family: "Inter"
                    color: "#FFFFFF"
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
            }
        }
        
        // Info text
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#3A3A3A"
            radius: 10
            border.color: "#555555"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 8
                
                Label {
                    Layout.fillWidth: true
                    text: "📋 This incident has been recorded:"
                    font.pixelSize: 14
                    font.family: "Inter"
                    font.bold: Font.Bold
                    color: "#FFAA00"
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "• Encrypted audit log entry created\n• Timestamp: " + new Date().toLocaleString()
                    font.pixelSize: 12
                    font.family: "Inter"
                    color: "#CCCCCC"
                    wrapMode: Text.WordWrap
                    lineHeight: 1.3
                }
            }
        }
        
        // Acknowledge button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            
            background: Rectangle {
                color: parent.hovered ? "#FF5555" : "#FF3333"
                radius: 12
                border.color: "#FF6666"
                border.width: 2
                
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            
            contentItem: Label {
                text: "I Understand"
                font.pixelSize: 18
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                warningDialog.close()
                acknowledged()
            }
        }
    }
}