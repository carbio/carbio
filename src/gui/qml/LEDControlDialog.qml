import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: ledDialog
    modal: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }
    
    contentItem: Rectangle {
        implicitWidth: 480
        implicitHeight: 450
        color: "transparent"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Label {
                text: "LED Control"
                font.pixelSize: 28
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#01E4E0"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#444444"
            }
            
            Label {
                text: "Select LED operation:"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#CCCCCC"
            }
            
            // LED ON Button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                
                contentItem: RowLayout {
                    spacing: 15
                    
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 6
                        color: "#1E1E1E"
                        border.color: "#32D74B"
                        border.width: 2
                        
                        Label {
                            anchors.centerIn: parent
                            text: "1"
                            font.pixelSize: 20
                            font.bold: Font.Bold
                            font.family: "Courier New"
                            color: "#32D74B"
                        }
                    }
                    
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Turn LED ON"
                            font.pixelSize: 18
                            font.family: "Inter"
                            font.bold: Font.DemiBold
                            color: "#FFFFFF"
                        }
                        
                        Label {
                            text: "Enable sensor LED light"
                            font.pixelSize: 13
                            font.family: "Inter"
                            color: "#AAAAAA"
                        }
                    }
                    
                    Canvas {
                        width: 30
                        height: 30
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "#32D74B"
                            ctx.beginPath()
                            ctx.arc(15, 15, 12, 0, 2 * Math.PI)
                            ctx.fill()
                        }
                    }
                }
                
                background: Rectangle {
                    color: parent.down ? "#2A4A2A" : (parent.hovered ? "#1E3A1E" : "#1A2A1A")
                    radius: 8
                    border.color: parent.hovered ? "#32D74B" : "#2A4A2A"
                    border.width: 2
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }
                
                onClicked: {
                    console.log("LED ON")
                    controller.turnLedOn()
                    ledDialog.close()
                }
            }
            
            // LED OFF Button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                
                contentItem: RowLayout {
                    spacing: 15
                    
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 6
                        color: "#1E1E1E"
                        border.color: "#666666"
                        border.width: 2
                        
                        Label {
                            anchors.centerIn: parent
                            text: "2"
                            font.pixelSize: 20
                            font.bold: Font.Bold
                            font.family: "Courier New"
                            color: "#666666"
                        }
                    }
                    
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Turn LED OFF"
                            font.pixelSize: 18
                            font.family: "Inter"
                            font.bold: Font.DemiBold
                            color: "#FFFFFF"
                        }
                        
                        Label {
                            text: "Disable sensor LED light"
                            font.pixelSize: 13
                            font.family: "Inter"
                            color: "#AAAAAA"
                        }
                    }
                    
                    Canvas {
                        width: 30
                        height: 30
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.strokeStyle = "#666666"
                            ctx.lineWidth = 3
                            ctx.beginPath()
                            ctx.arc(15, 15, 10, 0, 2 * Math.PI)
                            ctx.stroke()
                        }
                    }
                }
                
                background: Rectangle {
                    color: parent.down ? "#3A3A3A" : (parent.hovered ? "#353535" : "#2A2A2A")
                    radius: 8
                    border.color: parent.hovered ? "#666666" : "#3A3A3A"
                    border.width: 2
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }
                
                onClicked: {
                    console.log("LED OFF")
                    controller.turnLedOff()
                    ledDialog.close()
                }
            }
            
            Item { Layout.fillHeight: true }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                text: "Cancel"
                
                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 18
                    font.family: "Inter"
                    font.bold: Font.DemiBold
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    color: parent.down ? "#555555" : (parent.hovered ? "#4A4A4A" : "#3A3A3A")
                    radius: 8
                    border.color: "#666666"
                    border.width: 1
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                
                onClicked: ledDialog.close()
            }
        }
    }
}
