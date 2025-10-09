import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: infoDialog
    anchors.centerIn: parent
    width: 500
    height: Math.min(450, contentColumn.implicitHeight + 80)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    property string dialogTitle: "Information"
    property string dialogMessage: ""
    property bool isError: false
    
    // Dim background
    Overlay.modal: Rectangle {
        color: "#DD000000"
    }
    
    // Custom background
    background: Rectangle {
        id: dialogBox
        color: "#2A2A2A"
        radius: 20
        border.color: infoDialog.isError ? "#FF3333" : "#01E4E0"
        border.width: 3
        
        // Open animation
        scale: infoDialog.opened ? 1.0 : 0.8
        Behavior on scale {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutBack
            }
        }
    }
    
    function show(title, message, error) {
        dialogTitle = title || "Information"
        dialogMessage = message || ""
        isError = error || false
        open()
    }
    
    contentItem: ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20
        
        // Icon
        Canvas {
            Layout.alignment: Qt.AlignHCenter
            width: 60
            height: 60
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                
                var color = infoDialog.isError ? "#FF3333" : "#01E4E0"
                ctx.strokeStyle = color
                ctx.fillStyle = color
                ctx.lineWidth = 3
                ctx.lineCap = "round"
                
                if (infoDialog.isError) {
                    // Error X
                    ctx.beginPath()
                    ctx.moveTo(15, 15)
                    ctx.lineTo(45, 45)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(45, 15)
                    ctx.lineTo(15, 45)
                    ctx.stroke()
                } else {
                    // Info circle with 'i'
                    ctx.beginPath()
                    ctx.arc(30, 30, 25, 0, 2 * Math.PI)
                    ctx.stroke()
                    
                    // Dot
                    ctx.beginPath()
                    ctx.arc(30, 20, 3, 0, 2 * Math.PI)
                    ctx.fill()
                    
                    // Line
                    ctx.fillRect(27, 28, 6, 20)
                }
            }
            
            Component.onCompleted: requestPaint()
            
            Connections {
                target: infoDialog
                function onIsErrorChanged() {
                    parent.requestPaint()
                }
            }
        }
        
        // Title
        Label {
            Layout.fillWidth: true
            text: dialogTitle
            font.pixelSize: 22
            font.family: "Inter"
            font.bold: Font.Bold
            color: infoDialog.isError ? "#FF3333" : "#01E4E0"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
        
        // Message box
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            Layout.maximumHeight: 250
            color: "#1E1E1E"
            radius: 12
            border.color: infoDialog.isError ? "#FF6666" : "#444444"
            border.width: 1
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 15
                clip: true
                
                TextArea {
                    width: parent.width
                    text: dialogMessage
                    font.pixelSize: 14
                    font.family: "Consolas, monospace"
                    color: "#FFFFFF"
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                    background: Rectangle { color: "transparent" }
                }
            }
        }
        
        // Close button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            
            background: Rectangle {
                color: parent.hovered ? 
                    (infoDialog.isError ? "#FF5555" : "#00FFEE") : 
                    (infoDialog.isError ? "#FF3333" : "#01E4E0")
                radius: 10
                border.color: infoDialog.isError ? "#FF6666" : "#00FFEE"
                border.width: 2
                
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            
            contentItem: Label {
                text: "Close"
                font.pixelSize: 16
                font.family: "Inter"
                font.bold: Font.Bold
                color: infoDialog.isError ? "#FFFFFF" : "#1E1E1E"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: infoDialog.close()
        }
    }
}