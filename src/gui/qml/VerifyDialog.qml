import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Dialog {
    id: verifyDialog
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
        implicitHeight: controller.isProcessing ? 500 : 350
        color: "transparent"

        Behavior on implicitHeight {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Label {
                text: "Verify Fingerprint"
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
                text: "Enter ID # to verify (1-127):"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"
            }
            
            TextField {
                id: verifyIdInput
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
                    border.color: verifyIdInput.activeFocus ? "#01E4E0" : "#444444"
                    border.width: 2
                    
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }
                
                Keys.onReturnPressed: startVerify()
                Keys.onEnterPressed: startVerify()
            }
            
            Label {
                text: "Place finger on sensor to verify against specified ID"
                font.pixelSize: 14
                font.family: "Inter"
                color: "#CCCCCC"
                Layout.topMargin: 5
                visible: !controller.isProcessing
            }

            // Progress indicator (shown when processing)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                color: "#1E1E1E"
                radius: 8
                border.color: controller.isProcessing ? "#01E4E0" : "#444444"
                border.width: 2
                visible: controller.isProcessing

                Behavior on border.color { ColorAnimation { duration: 300 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15

                    ProgressRing {
                        Layout.alignment: Qt.AlignHCenter
                        width: 160
                        height: 160
                        progress: controller.scanProgress
                        ringColor: "#01E4E0"
                        centerText: ""
                    }

                    Label {
                        text: controller.operationProgress || "Verifying fingerprint..."
                        font.pixelSize: 16
                        font.family: "Inter"
                        font.bold: Font.DemiBold
                        color: "#01E4E0"
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        text: "Place finger on sensor"
                        font.pixelSize: 12
                        font.family: "Inter"
                        color: "#AAAAAA"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            Item { Layout.fillHeight: true }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Cancel"
                    enabled: !controller.isProcessing
                    
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
                        verifyIdInput.text = ""
                        verifyDialog.close()
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Verify"
                    enabled: !controller.isProcessing && verifyIdInput.text.length > 0 && parseInt(verifyIdInput.text) >= 1 && parseInt(verifyIdInput.text) <= 127
                    
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
                    
                    onClicked: startVerify()
                }
            }
        }
    }
    
    Connections {
        target: controller
        function onOperationComplete(message) {
            if (verifyDialog.visible && message.includes("Verification")) {
                verifyIdInput.text = ""
                verifyDialog.close()
            }
        }
        function onOperationFailed(error) {
            if (verifyDialog.visible) {
                // Keep dialog open on failure so user can retry
            }
        }
    }

    function startVerify() {
        if (verifyIdInput.text.length > 0) {
            var id = parseInt(verifyIdInput.text)
            if (id >= 1 && id <= 127) {
                console.log("Verifying ID:", id)
                controller.verifyFingerprint(id)
                // Keep dialog open to show progress
            }
        }
    }
    
    onOpened: {
        verifyIdInput.forceActiveFocus()
    }
}