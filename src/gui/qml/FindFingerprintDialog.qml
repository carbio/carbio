import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components/composites"

Dialog {
    id: findDialog
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }

    contentItem: Rectangle {
        implicitWidth: 500
        implicitHeight: 600
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20

            Label {
                text: "Find Fingerprint"
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

            // Real-time fingerprint scanner
            FingerprintScanner {
                id: fingerprintScanner
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 240
                Layout.preferredHeight: 300
                failedAttempts: 0
                isScanning: controller.isProcessing
                scanProgress: controller.scanProgress
            }

            // Status label
            Label {
                id: statusLabel
                Layout.fillWidth: true
                text: {
                    if (!controller.isProcessing && resultText !== "") {
                        return resultText
                    }
                    if (controller.scanProgress === 0) return "Place finger on sensor..."
                    else if (controller.scanProgress < 33) return "Capturing fingerprint image..."
                    else if (controller.scanProgress < 66) return "Extracting features..."
                    else if (controller.scanProgress < 100) return "Searching database..."
                    else return "Complete!"
                }
                font.pixelSize: 15
                font.family: "Inter"
                color: {
                    if (resultText.includes("ERROR") || resultText.includes("not found")) return "#FF6666"
                    if (resultText !== "" && !controller.isProcessing) return "#00FF88"
                    return controller.scanProgress > 0 ? "#01E4E0" : "#FFFFFF"
                }
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                Behavior on color { ColorAnimation { duration: 200 } }
            }

            // Progress percentage
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: controller.isProcessing && controller.scanProgress > 0 ? controller.scanProgress + "%" : ""
                font.pixelSize: 28
                font.family: "Courier New"
                font.bold: Font.Bold
                color: "#01E4E0"
                opacity: controller.isProcessing && controller.scanProgress > 0 ? 1.0 : 0.0

                Behavior on opacity { NumberAnimation { duration: 200 } }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 15

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: resultText !== "" ? "Close" : "Cancel"

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
                        resultText = ""
                        findDialog.close()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: resultText !== "" ? "Find Again" : "Start"
                    enabled: !controller.isProcessing
                    visible: resultText === "" || !resultText.includes("ERROR")

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

                    onClicked: {
                        resultText = ""
                        controller.findFingerprint()
                    }
                }
            }
        }
    }

    property string resultText: ""

    Connections {
        target: controller
        function onOperationComplete(message) {
            if (findDialog.visible && (message.includes("Template #") || message.includes("Found finger"))) {
                resultText = message
            }
        }
        function onOperationFailed(error) {
            if (findDialog.visible && error.includes("finger")) {
                resultText = error
            }
        }
    }

    onOpened: {
        resultText = ""
        if (!controller.isProcessing) {
            controller.findFingerprint()
        }
    }
}
