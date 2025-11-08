import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: clearDialog
    modal: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#FF3333"
        border.width: 3
        
        SequentialAnimation on border.color {
            running: clearDialog.opened
            loops: Animation.Infinite
            ColorAnimation { from: "#FF3333"; to: "#FF6666"; duration: 1000 }
            ColorAnimation { from: "#FF6666"; to: "#FF3333"; duration: 1000 }
        }
    }
    
    contentItem: Rectangle {
        implicitWidth: 550
        implicitHeight: 400
        color: "transparent"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 25
            
            Label {
                text: "Clear Database"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FF3333"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#663333"
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#3A1A1A"
                radius: 10
                border.color: "#FF3333"
                border.width: 2
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10
                    
                    Label {
                        text: "WARNING!"
                        font.pixelSize: 22
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: "#FF6666"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Label {
                        text: "This will permanently delete ALL fingerprints\nfrom the database. This action cannot be undone!"
                        font.pixelSize: 16
                        font.family: "Inter"
                        color: "#FFAAAA"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        lineHeight: 1.4
                    }
                }
            }
            
            Label {
                text: "Type 'y' to confirm:"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"
                Layout.topMargin: 10
            }
            
            TextField {
                id: confirmInput
                Layout.fillWidth: true
                Layout.preferredHeight: 55
                font.pixelSize: 32
                font.family: "Courier New"
                font.bold: Font.Bold
                horizontalAlignment: Text.AlignHCenter
                color: text === "y" ? "#FF3333" : "#FFFFFF"
                placeholderText: ""
                maximumLength: 1
                
                background: Rectangle {
                    color: "#1E1E1E"
                    radius: 8
                    border.color: confirmInput.activeFocus ? "#FF3333" : "#444444"
                    border.width: 3
                    
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }
                
                Keys.onReturnPressed: {
                    if (text === "y") startClear()
                }
                Keys.onEnterPressed: {
                    if (text === "y") startClear()
                }
            }
            
            Item { Layout.fillHeight: true }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 55
                    text: "Cancel"
                    
                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 20
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        color: parent.down ? "#555555" : (parent.hovered ? "#4A4A4A" : "#3A3A3A")
                        radius: 8
                        border.color: "#32D74B"
                        border.width: 2
                        
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    
                    onClicked: {
                        confirmInput.text = ""
                        clearDialog.close()
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 55
                    text: "Clear All"
                    enabled: confirmInput.text === "y"
                    
                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 20
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: parent.enabled ? "#FFFFFF" : "#666666"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        color: {
                            if (!parent.enabled) return "#2A2A2A"
                            if (parent.down) return "#AA0000"
                            if (parent.hovered) return "#FF5555"
                            return "#FF3333"
                        }
                        radius: 8
                        border.color: parent.enabled ? "#FF6666" : "#444444"
                        border.width: 2
                        
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    
                    onClicked: startClear()
                }
            }
        }
    }
    
    function startClear() {
        if (confirmInput.text === "y") {
            console.log("Clearing database")
            controller.clearDatabase()
            confirmInput.text = ""
            clearDialog.close()
        }
    }
    
    onOpened: {
        confirmInput.text = ""
        confirmInput.forceActiveFocus()
    }
}
