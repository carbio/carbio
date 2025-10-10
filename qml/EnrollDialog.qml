import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
            
            // Progress indicator (shown when processing)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#1E1E1E"
                radius: 8
                border.color: controller.isProcessing ? "#01E4E0" : "#444444"
                border.width: 2
                visible: controller.isProcessing

                Behavior on border.color { ColorAnimation { duration: 300 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    Label {
                        text: controller.operationProgress
                        font.pixelSize: 16
                        font.family: "Inter"
                        font.bold: Font.DemiBold
                        color: "#01E4E0"
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // Animated progress indicator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        color: "#2A2A2A"
                        radius: 2

                        Rectangle {
                            id: progressBar
                            height: parent.height
                            width: parent.width * 0.3
                            color: "#01E4E0"
                            radius: 2

                            SequentialAnimation on x {
                                running: controller.isProcessing
                                loops: Animation.Infinite
                                NumberAnimation {
                                    from: 0
                                    to: enrollDialog.width * 0.7
                                    duration: 1500
                                    easing.type: Easing.InOutQuad
                                }
                                NumberAnimation {
                                    from: enrollDialog.width * 0.7
                                    to: 0
                                    duration: 1500
                                    easing.type: Easing.InOutQuad
                                }
                            }
                        }
                    }

                    Label {
                        text: "Please wait..."
                        font.pixelSize: 12
                        font.family: "Inter"
                        color: "#AAAAAA"
                        Layout.alignment: Qt.AlignHCenter
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
                    enabled: !controller.isProcessing && idInput.text.length > 0 && parseInt(idInput.text) >= 1 && parseInt(idInput.text) <= 127
                    
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
        if (idInput.text.length > 0) {
            var id = parseInt(idInput.text)
            if (id >= 1 && id <= 127) {
                console.log("Starting enrollment for ID:", id)
                controller.enrollFingerprint(id)
                // Don't close dialog - let it stay open to show progress
            }
        }
    }
    
    onOpened: {
        idInput.forceActiveFocus()
    }
}
