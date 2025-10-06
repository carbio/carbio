import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: deleteDialog
    modal: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#FFA500"
        border.width: 2
    }
    
    contentItem: Rectangle {
        implicitWidth: 500
        implicitHeight: 350
        color: "transparent"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Label {
                text: "Delete Fingerprint"
                font.pixelSize: 28
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFA500"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#665500"
            }
            
            Label {
                text: "Enter ID # to delete (1-127):"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"
            }
            
            TextField {
                id: deleteIdInput
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                font.pixelSize: 24
                font.family: "Courier New"
                horizontalAlignment: Text.AlignHCenter
                color: "#FFFFFF"
                placeholderText: "ID"
                validator: IntValidator { bottom: 1; top: 127 }
                
                background: Rectangle {
                    color: "#1E1E1E"
                    radius: 8
                    border.color: deleteIdInput.activeFocus ? "#FFA500" : "#444444"
                    border.width: 2
                    
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }
                
                Keys.onReturnPressed: startDelete()
                Keys.onEnterPressed: startDelete()
            }
            
            Label {
                text: "This action cannot be undone"
                font.pixelSize: 14
                font.family: "Inter"
                color: "#FFA500"
                Layout.topMargin: 5
            }
            
            Item { Layout.fillHeight: true }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                
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
                    
                    onClicked: {
                        deleteIdInput.text = ""
                        deleteDialog.close()
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Delete"
                    enabled: deleteIdInput.text.length > 0 && parseInt(deleteIdInput.text) >= 1 && parseInt(deleteIdInput.text) <= 127
                    
                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 18
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: parent.enabled ? "#1E1E1E" : "#666666"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        color: {
                            if (!parent.enabled) return "#2A2A2A"
                            if (parent.down) return "#CC8800"
                            if (parent.hovered) return "#FFBB00"
                            return "#FFA500"
                        }
                        radius: 8
                        border.color: parent.enabled ? "#FFBB00" : "#444444"
                        border.width: 2
                        
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    
                    onClicked: startDelete()
                }
            }
        }
    }
    
    function startDelete() {
        if (deleteIdInput.text.length > 0) {
            var id = parseInt(deleteIdInput.text)
            if (id >= 1 && id <= 127) {
                console.log("Deleting ID:", id)
                controller.deleteFingerprint(id)
                deleteIdInput.text = ""
                deleteDialog.close()
            }
        }
    }
    
    onOpened: {
        deleteIdInput.forceActiveFocus()
    }
}
