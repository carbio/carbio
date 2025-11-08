import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components/composites"

Rectangle {
    id: setupWizard
    anchors.fill: parent
    color: "#1E1E1E"
    z: 200  // Above everything else

    property int currentStep: 1
    property int selectedRole: 1  // Default: PrimaryOwner for first user
    property string userName: ""

    ColumnLayout {
        anchors.centerIn: parent
        width: 700
        spacing: 30

        // Logo/Title
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            Label {
                text: "🚗"
                font.pixelSize: 80
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Welcome to CarBio"
                font.pixelSize: 36
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#01E4E0"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Vehicle Access Control System"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#AAAAAA"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#444444"
            Layout.topMargin: 20
            Layout.bottomMargin: 20
        }

        // Step 1: Welcome
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20
            visible: currentStep === 1

            Label {
                text: "First-Time Setup"
                font.pixelSize: 28
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFFFFF"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#2A2A2A"
                radius: 12
                border.color: "#01E4E0"
                border.width: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 25
                    spacing: 15

                    Label {
                        text: "No users are enrolled yet"
                        font.pixelSize: 18
                        font.family: "Inter"
                        font.bold: Font.DemiBold
                        color: "#FFFFFF"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "Let's set up your first owner account to get started.\nYou'll be able to add family members and other drivers later."
                        font.pixelSize: 14
                        font.family: "Inter"
                        color: "#AAAAAA"
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 300
                Layout.preferredHeight: 60

                contentItem: Label {
                    text: "Begin Setup →"
                    font.pixelSize: 20
                    font.family: "Inter"
                    font.bold: Font.Bold
                    color: "#1E1E1E"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.down ? "#00B8B0" : (parent.hovered ? "#00FFEE" : "#01E4E0")
                    radius: 12
                    border.color: "#00FFEE"
                    border.width: 2

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                onClicked: currentStep = 2
            }
        }

        // Step 2: Enter Name
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20
            visible: currentStep === 2

            Label {
                text: "Step 1 of 2: Your Information"
                font.pixelSize: 24
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFFFFF"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "What's your name?"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#AAAAAA"
                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: nameField
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                placeholderText: "Enter your full name"
                font.pixelSize: 20
                font.family: "Inter"
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter

                background: Rectangle {
                    color: "#2A2A2A"
                    radius: 12
                    border.color: nameField.activeFocus ? "#01E4E0" : "#444444"
                    border.width: 2

                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }

                Keys.onReturnPressed: {
                    if (nameField.text.trim().length > 0) {
                        userName = nameField.text.trim()
                        currentStep = 3
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 20
                spacing: 15

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "← Back"

                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 16
                        font.family: "Inter"
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: parent.down ? "#3A3A3A" : (parent.hovered ? "#4A4A4A" : "#2A2A2A")
                        radius: 10
                        border.color: "#666666"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    onClicked: currentStep = 1
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: controller.sensorInitializing ? "Initializing sensor..." : "Continue →"
                    enabled: nameField.text.trim().length > 0 &&
                             controller.sensorAvailable &&
                             !controller.sensorInitializing

                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 16
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
                        radius: 10
                        border.color: parent.enabled ? "#00FFEE" : "#444444"
                        border.width: 2

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    onClicked: {
                        userName = nameField.text.trim()
                        currentStep = 3
                        // Start enrollment immediately - no Qt.callLater needed
                        controller.enrollFirstBootUser(userName, selectedRole)
                    }
                }
            }
        }

        // Step 3: Enroll Fingerprint
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20
            visible: currentStep === 3

            Label {
                text: "Step 2 of 2: Fingerprint Enrollment"
                font.pixelSize: 24
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFFFFF"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "#2A2A2A"
                radius: 10
                border.color: "#01E4E0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Label {
                        text: "👤"
                        font.pixelSize: 32
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true

                        Label {
                            text: userName
                            font.pixelSize: 18
                            font.family: "Inter"
                            font.bold: Font.Bold
                            color: "#FFFFFF"
                        }

                        RowLayout {
                            spacing: 8

                            Rectangle {
                                width: 50
                                height: 18
                                radius: 3
                                color: "#FFD700"

                                Label {
                                    anchors.centerIn: parent
                                    text: "OWNER"
                                    font.pixelSize: 9
                                    font.bold: Font.Bold
                                    color: "#1E1E1E"
                                }
                            }

                            Label {
                                text: "Primary Owner • ID: 0"
                                font.pixelSize: 12
                                font.family: "Inter"
                                color: "#AAAAAA"
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 350
                color: "#1E1E1E"
                radius: 12
                border.color: controller.isProcessing ? "#01E4E0" : "#444444"
                border.width: 2

                Behavior on border.color { ColorAnimation { duration: 300 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 25
                    spacing: 20

                    FingerprintScanner {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 220
                        failedAttempts: 0
                        isScanning: controller.isProcessing
                        scanProgress: controller.scanProgress
                    }

                    Label {
                        text: controller.operationProgress || "Ready to begin"
                        font.pixelSize: 14
                        font.family: "Inter"
                        font.bold: Font.DemiBold
                        color: controller.isProcessing ? "#01E4E0" : "#AAAAAA"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.preferredHeight: 40
                    }

                    Label {
                        text: controller.scanProgress > 0 ? controller.scanProgress + "%" : ""
                        font.pixelSize: 32
                        font.family: "Courier New"
                        font.bold: Font.Bold
                        color: "#01E4E0"
                        Layout.alignment: Qt.AlignHCenter
                        opacity: controller.scanProgress > 0 ? 1.0 : 0.0

                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }
                }
            }

            // Status message during enrollment (simplified - avoid redundant bindings)
            Label {
                Layout.fillWidth: true
                Layout.topMargin: 20
                text: controller.isProcessing ? "Processing..." : "Ready"
                font.pixelSize: 14
                font.family: "Inter"
                color: controller.isProcessing ? "#01E4E0" : "#AAAAAA"
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.8
            }

            // Back button (only enabled when not enrolling)
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                Layout.topMargin: 10
                text: "← Back"
                enabled: !controller.isProcessing
                visible: !controller.isProcessing

                contentItem: Label {
                    text: parent.text
                    font.pixelSize: 16
                    font.family: "Inter"
                    color: parent.enabled ? "#FFFFFF" : "#666666"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.down ? "#3A3A3A" : (parent.hovered ? "#4A4A4A" : "#2A2A2A")
                    radius: 10
                    border.color: "#666666"
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                onClicked: currentStep = 2
            }
        }
    }

    Connections {
        target: controller
        function onOperationComplete(message) {
            // Check if this is the first enrollment completion
            if (message.includes("enrolled successfully") && controller.userManager.count === 1) {
                console.log("First enrollment complete - starting authentication")
                controller.resetFailedAttempts()

                // Start authentication immediately
                // The wizard will auto-hide via binding: visible = count === 0
                // The lock screen will show via binding: visible = authState !== ON && count > 0
                controller.startAuthentication()
            }
        }
    }

    function reset() {
        currentStep = 1
        userName = ""
        selectedRole = 1
        nameField.text = ""
    }
}
