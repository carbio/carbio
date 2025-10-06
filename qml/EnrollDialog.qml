import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: enrollDialog
    modal: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }
    
    contentItem: Rectangle {
        implicitWidth: 500
        implicitHeight: 400
        color: "transparent"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Label {
                text: "Enroll Fingerprint"
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
                text: "Enter ID # from 1-127:"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"
            }
            
            TextField {
                id: idInput
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
                    border.color: idInput.activeFocus ? "#01E4E0" : "#444444"
                    border.width: 2
                    
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }
                
                Keys.onReturnPressed: startEnroll()
                Keys.onEnterPressed: startEnroll()
            }
            
            Label {
                text: "Process:"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#AAAAAA"
                Layout.topMargin: 10
            }
            
            Label {
                text: "1. Scan finger first time\n2. Remove finger\n3. Scan same finger again\n4. Create and store template"
                font.pixelSize: 14
                font.family: "Inter"
                color: "#CCCCCC"
                lineHeight: 1.4
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
                        idInput.text = ""
                        enrollDialog.close()
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Start Enroll"
                    enabled: idInput.text.length > 0 && parseInt(idInput.text) >= 1 && parseInt(idInput.text) <= 127
                    
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
                            if (parent.down) return "#00B8B0"
                            if (parent.hovered) return "#00FFEE"
                            return "#01E4E0"
                        }
                        radius: 8
                        border.color: parent.enabled ? "#00FFEE" : "#444444"
                        border.width: 2
                        
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    
                    onClicked: startEnroll()
                }
            }
        }
    }
    
    function startEnroll() {
        if (idInput.text.length > 0) {
            var id = parseInt(idInput.text)
            if (id >= 1 && id <= 127) {
                console.log("Starting enrollment for ID:", id)
                controller.enrollFingerprint(id)
                idInput.text = ""
                enrollDialog.close()
            }
        }
    }
    
    onOpened: {
        idInput.forceActiveFocus()
    }
}
