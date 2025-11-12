import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "components/composites"

Dialog {
    id: enrollDialog
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose

    property int selectedRole: 3  // Default: FamilyMember
    property int assignedId: -1

    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }

    contentItem: Rectangle {
        implicitWidth: 650
        implicitHeight: 750
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 15

            Label {
                text: "Add New User"
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

            // Step 1: Name Input
            Label {
                text: "User Name:"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#FFFFFF"
                Layout.topMargin: 5
            }

            TextField {
                id: nameInput
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                placeholderText: "Enter name (e.g., John Doe)"
                font.pixelSize: 18
                font.family: "Inter"
                color: "#FFFFFF"

                background: Rectangle {
                    color: "#1E1E1E"
                    radius: 8
                    border.color: nameInput.activeFocus ? "#01E4E0" : "#444444"
                    border.width: 2

                    Behavior on border.color { ColorAnimation { duration: 200 } }
                }

                Keys.onReturnPressed: {
                    if (canEnroll()) {
                        startEnroll()
                    }
                }
            }

            // Step 2: Role Selection
            Label {
                text: "Select Role:"
                font.pixelSize: 16
                font.family: "Inter"
                color: "#FFFFFF"
                Layout.topMargin: 10
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 10
                columnSpacing: 10

                RoleCard {
                    roleName: "Primary Owner"
                    roleIcon: "👑"
                    roleColor: "#FFD700"
                    description: "Full access + admin"
                    roleValue: 1
                    selected: enrollDialog.selectedRole === 1
                    onClicked: enrollDialog.selectedRole = 1
                }

                RoleCard {
                    roleName: "Secondary Owner"
                    roleIcon: "⭐"
                    roleColor: "#01E4E0"
                    description: "Admin access"
                    roleValue: 2
                    selected: enrollDialog.selectedRole === 2
                    onClicked: enrollDialog.selectedRole = 2
                }

                RoleCard {
                    roleName: "Family Member"
                    roleIcon: "👨‍👩‍👧"
                    roleColor: "#32D74B"
                    description: "Drive + settings"
                    roleValue: 3
                    selected: enrollDialog.selectedRole === 3
                    onClicked: enrollDialog.selectedRole = 3
                }

                RoleCard {
                    roleName: "Restricted"
                    roleIcon: "🚗"
                    roleColor: "#FFB340"
                    description: "Drive only"
                    roleValue: 4
                    selected: enrollDialog.selectedRole === 4
                    onClicked: enrollDialog.selectedRole = 4
                }

                RoleCard {
                    roleName: "Passenger"
                    roleIcon: "🧑‍🤝‍🧑"
                    roleColor: "#0A84FF"
                    description: "Climate + media"
                    roleValue: 5
                    selected: enrollDialog.selectedRole === 5
                    onClicked: enrollDialog.selectedRole = 5
                }

                RoleCard {
                    roleName: "Service Tech"
                    roleIcon: "🔧"
                    roleColor: "#BF5AF2"
                    description: "Maintenance"
                    roleValue: 6
                    selected: enrollDialog.selectedRole === 6
                    onClicked: enrollDialog.selectedRole = 6
                }
            }

            // Auto-assigned ID display
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "#1E3E1E"
                radius: 8
                border.color: "#32D74B"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: "📋"
                        font.pixelSize: 20
                    }

                    Label {
                        text: "Auto-assigned ID:"
                        font.pixelSize: 14
                        font.family: "Inter"
                        color: "#32D74B"
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 35
                        radius: 6
                        color: "#2A4A2A"
                        border.color: "#32D74B"
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: controller.userManager.findNextAvailableId()
                            font.pixelSize: 18
                            font.family: "Courier New"
                            font.bold: Font.Bold
                            color: "#32D74B"
                        }
                    }
                }
            }

            // Progress indicator (shown when processing)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 250
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
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 180
                        failedAttempts: 0
                        isScanning: controller.isProcessing
                        scanProgress: controller.scanProgress
                    }

                    Label {
                        text: controller.operationProgress || "Ready to begin..."
                        font.pixelSize: 14
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

            Item { Layout.fillHeight: true }

            // Action buttons
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
                        nameInput.text = ""
                        enrollDialog.selectedRole = 3
                        enrollDialog.close()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Enroll Fingerprint"
                    enabled: canEnroll()

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

    // Role Card Component
    component RoleCard: Rectangle {
        property string roleName
        property string roleIcon
        property color roleColor
        property string description
        property int roleValue
        property bool selected: false

        signal clicked()

        Layout.fillWidth: true
        height: 90
        color: selected ? roleColor : "#2A2A2A"
        radius: 10
        border.color: selected ? roleColor : "#444444"
        border.width: 2

        Behavior on color { ColorAnimation { duration: 200 } }
        Behavior on border.color { ColorAnimation { duration: 200 } }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 5

            Label {
                text: roleIcon
                font.pixelSize: 28
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: roleName
                font.pixelSize: 13
                font.family: "Inter"
                font.bold: Font.DemiBold
                color: selected ? "#1E1E1E" : "#FFFFFF"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: description
                font.pixelSize: 10
                font.family: "Inter"
                color: selected ? "#1E1E1E" : "#AAAAAA"
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    Connections {
        target: controller
        function onOperationComplete(message) {
            if (enrollDialog.visible && message.includes("enrolled successfully")) {
                nameInput.text = ""
                enrollDialog.selectedRole = 3
                enrollDialog.close()
            }
        }
    }

    function canEnroll() {
        return !controller.isProcessing &&
               nameInput.text.trim().length > 0 &&
               controller.userManager.findNextAvailableId() >= 0
    }

    function startEnroll() {
        if (!canEnroll()) {
            return
        }
        var id = controller.userManager.addUser(nameInput.text.trim(), selectedRole)
        if (id < 0) {
            console.error("Failed to add user - no available IDs")
            return
        }
        assignedId = id
        controller.enrollFingerprint(id)
    }

    onOpened: {
        nameInput.forceActiveFocus()
        nameInput.text = ""
        selectedRole = 3  // Reset to FamilyMember
    }
}
