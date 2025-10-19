import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "components/composites"

Dialog {
    id: enrollDialog
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose // Prevent closing during operation
    
    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }
    
    contentItem: Rectangle {
        implicitWidth: 550
        implicitHeight: 650
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
                text: "Fingerprint ID (0-127, admin: 0-2):"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"
                Layout.topMargin: 10
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
                validator: IntValidator { bottom: 0; top: 127 }

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

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: "#1E1E1E"
                radius: 8
                border.color: "#444444"
                border.width: 1

                Label {
                    anchors.fill: parent
                    anchors.margins: 15
                    text: "Note: Admin privileges are automatically assigned to IDs 0-2 by convention (zero-configuration security)."
                    font.pixelSize: 14
                    font.family: "Inter"
                    color: "#AAAAAA"
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            // Progress indicator (shown when processing)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 350
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

                    FingerprintScanner {
                        id: fingerprintScanner
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 250
                        failedAttempts: 0
                        isScanning: controller.isProcessing
                        scanProgress: controller.scanProgress
                    }

                    Label {
                        text: {
                            if (controller.scanProgress === 0) return "Place finger on sensor..."
                            else if (controller.scanProgress < 25) return "Capturing first scan..."
                            else if (controller.scanProgress < 50) return "Remove finger now"
                            else if (controller.scanProgress < 75) return "Place same finger again..."
                            else if (controller.scanProgress < 100) return "Creating template..."
                            else return "Enrollment complete!"
                        }
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
                        text: controller.scanProgress > 0 ? controller.scanProgress + "%" : ""
                        font.pixelSize: 24
                        font.family: "Courier New"
                        font.bold: Font.Bold
                        color: "#01E4E0"
                        Layout.alignment: Qt.AlignHCenter
                        opacity: controller.scanProgress > 0 ? 1.0 : 0.0

                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }
                }
            }

            // Instructions (shown when NOT processing)
            ColumnLayout {
                spacing: 8
                visible: !controller.isProcessing

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
                        color: parent.enabled ? "#FFFFFF" : "#666666"
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
                    enabled: !controller.isProcessing &&
                             idInput.text.length > 0 &&
                             parseInt(idInput.text) >= 0 &&
                             parseInt(idInput.text) <= 127
                    
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
    
    Connections {
        target: controller
        function onOperationComplete(message) {
            // Close dialog on successful enrollment
            if (enrollDialog.visible && message.includes("enrolled successfully")) {
                idInput.text = ""
                enrollDialog.close()
            }
        }
        function onOperationFailed(error) {
            // Keep dialog open on failure so user can retry
        }
    }

    function startEnroll() {
        var id = parseInt(idInput.text)

        if (id >= 0 && id <= 127) {
            console.log("Enrolling fingerprint at ID:", id, "(Admin:", id <= 2, ")")
            controller.enrollFingerprint(id)
            // Dialog stays open to show progress
        }
    }

    onOpened: {
        idInput.forceActiveFocus()
        idInput.text = ""
    }
}
