import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: fingerprintSetupDialog
    anchors.fill: parent
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
            spacing: 75
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                // Title with fingerprint icon
                Rectangle {
                    Layout.alignment: Qt.AlignHLeft
                    color: "transparent"
                    anchors.left: parent.left

                    Label {
                        text: "FINGERPRINT SETUP"
                        font.pixelSize: 32
                        font.family: "Inter"
                        font.bold: Font.Bold
                        color: "#01E4E0"
                    }
                }

                // Close button
                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: 15
                    color: closeMouseArea.containsMouse ? "#4A4A4A" : "#3A3A3A"
                    border.color: "#666666"
                    border.width: 1
                    anchors.right: parent.right

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

            // Template count display
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
                        anchors.centerIn: parent
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
                            anchors.left: parent.left
                            color: controller.templateCount > 0 ? "#1E1E1E" : "#AAAAAA"

                            Behavior on color { ColorAnimation { duration: 200 } }
                        }
                    }

                    // Refresh button
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

                                // Draw refresh arrow
                                var centerX = width / 2
                                var centerY = height / 2
                                var radius = 10

                                ctx.beginPath()
                                ctx.arc(centerX, centerY, radius, -Math.PI / 4, Math.PI * 1.5, false)
                                ctx.stroke()

                                // Arrow head
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
            
            // Scrollable menu items
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: parent.parent.width
                    spacing: 10
                    
                    MenuButton {
                        text: "Enroll Print"
                        description: "Register new fingerprint (ID 1-127)"
                        onClicked: enrollDialog.open()
                    }

                    MenuButton {
                        text: "Find Print"
                        description: "Scan and find fingerprint details"
                        onClicked: controller.findFingerprint()
                    }

                    MenuButton {
                        text: "Identify Print"
                        description: "Identify without knowing ID"
                        onClicked: controller.identifyFingerprint()
                    }

                    MenuButton {
                        text: "Verify Print"
                        description: "Verify specific ID"
                        onClicked: verifyDialog.open()
                    }

                    MenuButton {
                        text: "Query Print"
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
                        description: "Remove fingerprint by ID"
                        textColor: "#FFA500"
                        onClicked: deleteDialog.open()
                    }

                    MenuButton {
                        text: "Clear Database"
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
                        description: "Sensor LED on/off/toggle"
                        onClicked: ledDialog.open()
                    }

                    MenuButton {
                        text: "System Config"
                        description: "Baud rate, security, settings"
                        onClicked: configDialog.open()
                    }

                    MenuButton {
                        text: "Soft Reset"
                        description: "Reset sensor to defaults"
                        onClicked: controller.softResetSensor()
                    }
                }
            }
        }
        
        // Dialogs
        EnrollDialog {
            id: enrollDialog
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
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15
                
                Rectangle {
                    Layout.preferredWidth: 35
                    Layout.preferredHeight: 35
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
                
                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    
                    Label {
                        text: parent.parent.parent.text
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
    }

    function cancelDialog() {
        close()
    }
}
