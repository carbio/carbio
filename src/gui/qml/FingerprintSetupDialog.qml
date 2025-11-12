import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: fingerprintSetupDialog
    anchors.centerIn: parent
    color: "#2A2A2A"
    radius: 15
    border.color: "#01E4E0"
    border.width: 2
    visible: false
    
    signal closed()

    function open() {
        visible = true
        openAnimation.start()
    }

    function close() {
        closeAnimation.start()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    Rectangle {
        id: dialogBox
        anchors.centerIn: parent
        width: 800
        height: 750
        color: "#2A2A2A"
        radius: 20
        border.color: "#01E4E0"
        border.width: 3
        scale: 0.8
        opacity: 0

        NumberAnimation {
            id: openAnimation
            target: dialogBox
            properties: "scale,opacity"
            to: 1.0
            duration: 300
            easing.type: Easing.OutBack
        }

        NumberAnimation {
            id: closeAnimation
            target: dialogBox
            properties: "scale,opacity"
            to: 0.8
            duration: 200
            easing.type: Easing.InQuad
            onFinished: {
                fingerprintSetupDialog.visible = false
                closed()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                
                Label {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "FINGERPRINT SETUP"
                    font.pixelSize: 32
                    font.family: "Inter"
                    font.bold: Font.Bold
                    color: "#01E4E0"
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30
                    height: 30
                    radius: 15
                    color: closeMouseArea.containsMouse ? "#4A4A4A" : "#3A3A3A"
                    border.color: "#666666"
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }

                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fingerprintSetupDialog.close()
                    }

                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.strokeStyle = "#CCCCCC"
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"

                            var margin = 9
                            var size = 12

                            ctx.beginPath()
                            ctx.moveTo(margin, margin)
                            ctx.lineTo(margin + size, margin + size)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(margin + size, margin)
                            ctx.lineTo(margin, margin + size)
                            ctx.stroke()
                        }
                    }
                }
            }

            // Session timer display
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "#1E3E1E"
                radius: 15
                border.color: "#32D74B"
                border.width: 2
                visible: controller.adminSession.isActive

                Behavior on opacity { NumberAnimation { duration: 300 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Label {
                        text: "🔓"
                        font.pixelSize: 20
                    }

                    Label {
                        text: "Admin Session Active"
                        font.pixelSize: 14
                        font.family: "Inter"
                        font.bold: Font.DemiBold
                        color: "#32D74B"
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 35
                        radius: 6
                        color: "#2A4A2A"
                        border.color: "#32D74B"
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: {
                                var secs = controller.adminSession.remainingSeconds
                                var mins = Math.floor(secs / 60)
                                var seconds = secs % 60
                                return mins + ":" + (seconds < 10 ? "0" : "") + seconds
                            }
                            font.pixelSize: 16
                            font.family: "Courier New"
                            font.bold: Font.Bold
                            color: "#32D74B"
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "#1E1E1E"
                radius: 15
                border.color: "#01E4E0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Label {
                        text: "Templates Stored:"
                        font.pixelSize: 16
                        font.family: "Inter"
                        color: "#AAAAAA"
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 35
                        radius: 6
                        color: controller.templateCount > 0 ? "#01E4E0" : "#3A3A3A"
                        border.color: controller.templateCount > 0 ? "#00FFEE" : "#555555"
                        border.width: 2

                        Behavior on color { ColorAnimation { duration: 200 } }

                        Label {
                            anchors.centerIn: parent
                            text: controller.templateCount
                            font.pixelSize: 18
                            font.family: "Courier New"
                            font.bold: Font.Bold
                            color: controller.templateCount > 0 ? "#1E1E1E" : "#AAAAAA"

                            Behavior on color { ColorAnimation { duration: 200 } }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 35
                        Layout.preferredHeight: 35
                        radius: 6
                        color: refreshMouseArea.containsMouse ? "#3A4A4A" : "#2A3A3A"
                        border.color: "#01E4E0"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        MouseArea {
                            id: refreshMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: controller.refreshTemplateCount()
                        }

                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.strokeStyle = "#01E4E0"
                                ctx.lineWidth = 2
                                ctx.lineCap = "round"

                                var centerX = width / 2
                                var centerY = height / 2
                                var radius = 10

                                ctx.beginPath()
                                ctx.arc(centerX, centerY, radius, -Math.PI / 4, Math.PI * 1.5, false)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(centerX + radius * 0.7, centerY - radius * 0.7)
                                ctx.lineTo(centerX + radius * 0.7 + 4, centerY - radius * 0.7 - 4)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(centerX + radius * 0.7, centerY - radius * 0.7)
                                ctx.lineTo(centerX + radius * 0.7 + 4, centerY - radius * 0.7 + 4)
                                ctx.stroke()
                            }
                        }
                    }
                }
            }

            // Tab bar for Users vs Operations
            TabBar {
                id: tabBar
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                background: Rectangle {
                    color: "transparent"
                }

                TabButton {
                    text: "👥 Users"
                    font.pixelSize: 16
                    font.family: "Inter"

                    contentItem: Label {
                        text: parent.text
                        font: parent.font
                        color: parent.checked ? "#01E4E0" : "#AAAAAA"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        Behavior on color { ColorAnimation { duration: 200 } }
                    }

                    background: Rectangle {
                        color: parent.checked ? "#1E3E3E" : "transparent"
                        radius: 8
                        border.color: parent.checked ? "#01E4E0" : "transparent"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 200 } }
                        Behavior on border.color { ColorAnimation { duration: 200 } }
                    }
                }

                TabButton {
                    text: "⚙️ Operations"
                    font.pixelSize: 16
                    font.family: "Inter"

                    contentItem: Label {
                        text: parent.text
                        font: parent.font
                        color: parent.checked ? "#01E4E0" : "#AAAAAA"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        Behavior on color { ColorAnimation { duration: 200 } }
                    }

                    background: Rectangle {
                        color: parent.checked ? "#1E3E3E" : "transparent"
                        radius: 8
                        border.color: parent.checked ? "#01E4E0" : "transparent"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 200 } }
                        Behavior on border.color { ColorAnimation { duration: 200 } }
                    }
                }
            }

            // Tab content
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // Tab 1: Users List
                UserListView {
                    onDeleteRequested: function(userId, userName) {
                        deleteConfirmDialog.userId = userId
                        deleteConfirmDialog.userName = userName
                        deleteConfirmDialog.open()
                    }
                }

                // Tab 2: Operations Menu
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.parent.width
                        spacing: 10

                        MenuButton {
                            text: "Enroll Print"
                            keyText: "1"
                            description: "Register new user with name and role"
                            onClicked: enrollDialog.open()
                        }

                    MenuButton {
                        text: "Find Print"
                        keyText: "2"
                        description: "Scan and find fingerprint details"
                        onClicked: findDialog.open()
                    }

                    MenuButton {
                        text: "Identify Print"
                        keyText: "3"
                        description: "Identify without knowing ID"
                        onClicked: identifyDialog.open()
                    }

                    MenuButton {
                        text: "Verify Print"
                        keyText: "4"
                        description: "Verify specific ID"
                        onClicked: verifyDialog.open()
                    }

                    MenuButton {
                        text: "Query Print"
                        keyText: "5"
                        description: "Check if template exists"
                        onClicked: queryDialog.open()
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444444"
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                    }
                    
                    MenuButton {
                        text: "Delete Print"
                        keyText: "6"
                        description: "Remove fingerprint by ID"
                        textColor: "#FFA500"
                        onClicked: deleteDialog.open()
                    }

                    MenuButton {
                        text: "Clear Database"
                        keyText: "7"
                        description: "Delete ALL fingerprints"
                        textColor: "#FF6666"
                        onClicked: clearDialog.open()
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444444"
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                    }
                    
                    MenuButton {
                        text: "LED Control"
                        keyText: "8"
                        description: "Sensor LED on/off/toggle"
                        onClicked: ledDialog.open()
                    }

                    MenuButton {
                        text: "System Config"
                        keyText: "9"
                        description: "Baud rate, security, settings"
                        onClicked: configDialog.open()
                    }

                    MenuButton {
                        text: "Soft Reset"
                        keyText: "0"
                        description: "Reset sensor to defaults"
                        onClicked: controller.softResetSensor()
                    }
                    }
                }
            }
        }

        // Delete Confirmation Dialog
        Dialog {
            id: deleteConfirmDialog
            modal: true
            anchors.centerIn: parent

            property int userId: -1
            property string userName: ""

            background: Rectangle {
                color: "#2A2A2A"
                radius: 12
                border.color: "#FF6666"
                border.width: 2
            }

            contentItem: Rectangle {
                implicitWidth: 450
                implicitHeight: 250
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 25
                    spacing: 20

                    Label {
                        text: "⚠️ Delete User"
                        font.pixelSize: 24
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: "#FF6666"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#666666"
                    }

                    Label {
                        text: "Are you sure you want to delete this user?"
                        font.pixelSize: 16
                        font.family: "Inter"
                        color: "#FFFFFF"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        color: "#3A2A2A"
                        radius: 8
                        border.color: "#FF6666"
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4

                            Label {
                                text: deleteConfirmDialog.userName
                                font.pixelSize: 16
                                font.family: "Inter"
                                font.bold: Font.Bold
                                color: "#FFFFFF"
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text: "ID: " + deleteConfirmDialog.userId
                                font.pixelSize: 12
                                font.family: "Courier New"
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
                            Layout.preferredHeight: 45
                            text: "Cancel"

                            contentItem: Label {
                                text: parent.text
                                font.pixelSize: 16
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

                            onClicked: deleteConfirmDialog.close()
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 45
                            text: "Delete"

                            contentItem: Label {
                                text: parent.text
                                font.pixelSize: 16
                                font.family: "Inter"
                                font.bold: Font.Bold
                                color: "#FFFFFF"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: parent.down ? "#CC4444" : (parent.hovered ? "#EE5555" : "#FF6666")
                                radius: 8
                                border.color: "#FF8888"
                                border.width: 1

                                Behavior on color { ColorAnimation { duration: 150 } }
                            }

                            onClicked: {
                                controller.deleteFingerprint(deleteConfirmDialog.userId)
                                deleteConfirmDialog.close()
                            }
                        }
                    }
                }
            }
        }

        // Dialogs
        EnrollDialog {
            id: enrollDialog
        }

        FindFingerprintDialog {
            id: findDialog
        }

        IdentifyFingerprintDialog {
            id: identifyDialog
        }

        VerifyDialog {
            id: verifyDialog
        }

        QueryDialog {
            id: queryDialog
        }

        DeleteDialog {
            id: deleteDialog
        }

        ClearConfirmDialog {
            id: clearDialog
        }

        LEDControlDialog {
            id: ledDialog
        }

        SystemConfigDialog {
            id: configDialog
        }
        
        component MenuButton: Rectangle {
            property string text: ""
            property string keyText: ""
            property string description: ""
            property color textColor: "#FFFFFF"

            signal clicked()

            Layout.fillWidth: true
            height: 70
            color: "#3A3A3A"
            radius: 8
            border.color: mouseArea.containsMouse ? "#01E4E0" : "transparent"
            border.width: 2

            scale: mouseArea.pressed ? 0.97 : 1.0

            Behavior on border.color { ColorAnimation { duration: 150 } }
            Behavior on color { ColorAnimation { duration: 150 } }
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onEntered: parent.color = "#4A4A4A"
                onExited: parent.color = "#3A3A3A"
                onClicked: parent.clicked()
            }
            
            Rectangle {
                id: keyBox
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                width: 35
                height: 35
                radius: 6
                color: "#1E1E1E"
                border.color: "#01E4E0"
                border.width: 1
                
                Label {
                    anchors.centerIn: parent
                    text: keyText
                    font.pixelSize: 18
                    font.bold: Font.Bold
                    font.family: "Courier New"
                    color: "#01E4E0"
                }
            }
            
            Column {
                anchors.left: keyBox.right
                anchors.leftMargin: 15
                anchors.right: parent.right
                anchors.rightMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                
                Label {
                    text: parent.parent.text
                    font.pixelSize: 18
                    font.family: "Inter"
                    font.bold: Font.DemiBold
                    color: textColor
                }
                
                Label {
                    text: description
                    font.pixelSize: 13
                    font.family: "Inter"
                    color: "#AAAAAA"
                    opacity: 0.8
                }
            }
        }
    }

    function cancelDialog() {
        close()
    }
}