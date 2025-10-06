import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: configDialog
    modal: true
    anchors.centerIn: parent

    // Entrance animation
    enter: Transition {
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            duration: 250
            easing.type: Easing.OutBack
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            duration: 150
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 150
        }
    }

    background: Rectangle {
        color: "#2A2A2A"
        radius: 12
        border.color: "#01E4E0"
        border.width: 2
    }
    
    contentItem: Rectangle {
        implicitWidth: 600
        implicitHeight: 500
        color: "transparent"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 25
            spacing: 20
            
            Label {
                text: "System Config"
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
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: parent.parent.width
                    spacing: 12

                    Button {
                        Layout.fillWidth: true
                        height: 70

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: RowLayout {
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
                                    text: "1"
                                    font.pixelSize: 16
                                    font.bold: Font.Bold
                                    font.family: "Courier New"
                                    color: "#01E4E0"
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                Label {
                                    text: "Baud Rate"
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#FFFFFF"
                                }

                                Label {
                                    text: "Change sensor communication speed"
                                    font.pixelSize: 12
                                    font.family: "Inter"
                                    color: "#AAAAAA"
                                }
                            }
                        }

                        background: Rectangle {
                            color: parent.down ? "#3A4A4A" : (parent.hovered ? "#3A3A3A" : "#2A2A2A")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "transparent"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }
                        }

                        onClicked: baudDialog.open()
                    }

                    Button {
                        Layout.fillWidth: true
                        height: 70

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: RowLayout {
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
                                    text: "2"
                                    font.pixelSize: 16
                                    font.bold: Font.Bold
                                    font.family: "Courier New"
                                    color: "#01E4E0"
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                Label {
                                    text: "Security Level"
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#FFFFFF"
                                }

                                Label {
                                    text: "Adjust fingerprint matching threshold"
                                    font.pixelSize: 12
                                    font.family: "Inter"
                                    color: "#AAAAAA"
                                }
                            }
                        }

                        background: Rectangle {
                            color: parent.down ? "#3A4A4A" : (parent.hovered ? "#3A3A3A" : "#2A2A2A")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "transparent"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }
                        }

                        onClicked: securityDialog.open()
                    }

                    Button {
                        Layout.fillWidth: true
                        height: 70

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: RowLayout {
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
                                    text: "3"
                                    font.pixelSize: 16
                                    font.bold: Font.Bold
                                    font.family: "Courier New"
                                    color: "#01E4E0"
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                Label {
                                    text: "Packet Size"
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#FFFFFF"
                                }

                                Label {
                                    text: "Configure packet transmission size"
                                    font.pixelSize: 12
                                    font.family: "Inter"
                                    color: "#AAAAAA"
                                }
                            }
                        }

                        background: Rectangle {
                            color: parent.down ? "#3A4A4A" : (parent.hovered ? "#3A3A3A" : "#2A2A2A")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "transparent"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }
                        }

                        onClicked: packetDialog.open()
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        height: 70
                        enabled: false
                        
                        contentItem: RowLayout {
                            spacing: 15
                            
                            Rectangle {
                                Layout.preferredWidth: 35
                                Layout.preferredHeight: 35
                                radius: 6
                                color: "#1E1E1E"
                                border.color: "#444444"
                                border.width: 1
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: "4"
                                    font.pixelSize: 16
                                    font.bold: Font.Bold
                                    font.family: "Courier New"
                                    color: "#666666"
                                }
                            }
                            
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                
                                Label {
                                    text: "Device Password"
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#666666"
                                }
                                
                                Label {
                                    text: "Not implemented in module"
                                    font.pixelSize: 12
                                    font.family: "Inter"
                                    color: "#AAAAAA"
                                }
                            }
                        }
                        
                        background: Rectangle {
                            color: "#1A1A1A"
                            radius: 8
                            border.color: "transparent"
                            border.width: 1
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        height: 70

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: RowLayout {
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
                                    text: "5"
                                    font.pixelSize: 16
                                    font.bold: Font.Bold
                                    font.family: "Courier New"
                                    color: "#01E4E0"
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                Label {
                                    text: "Check System Config"
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#FFFFFF"
                                }

                                Label {
                                    text: "View current system config"
                                    font.pixelSize: 12
                                    font.family: "Inter"
                                    color: "#AAAAAA"
                                }
                            }
                        }

                        background: Rectangle {
                            color: parent.down ? "#3A4A4A" : (parent.hovered ? "#3A3A3A" : "#2A2A2A")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "transparent"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }
                        }

                        onClicked: {
                            controller.showSystemSettings()
                        }
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                text: "Close"

                scale: down ? 0.97 : 1.0
                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

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

                onClicked: configDialog.close()
            }
        }
    }
    
    // Baud Rate Dialog
    Dialog {
        id: baudDialog
        modal: true
        anchors.centerIn: parent

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.9; to: 1.0; duration: 250; easing.type: Easing.OutBack }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.9; duration: 150; easing.type: Easing.InQuad }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
        }

        background: Rectangle {
            color: "#2A2A2A"
            radius: 12
            border.color: "#01E4E0"
            border.width: 2
        }
        
        contentItem: Rectangle {
            implicitWidth: 550
            implicitHeight: 500
            color: "transparent"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 25
                spacing: 15
                
                Label {
                    text: "Select Baud Rate"
                    font.pixelSize: 24
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
                    text: "Select communication speed between host and sensor:"
                    font.pixelSize: 14
                    font.family: "Inter"
                    color: "#CCCCCC"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    
                    ColumnLayout {
                        width: parent.parent.width
                        spacing: 8
                        
                        Repeater {
                            model: [
                                { value: "9600", num: 1 },
                                { value: "19200", num: 2 },
                                { value: "28800", num: 3 },
                                { value: "38400", num: 4 },
                                { value: "48000", num: 5 },
                                { value: "57600", num: 6 },
                                { value: "67200", num: 7 },
                                { value: "76800", num: 8 },
                                { value: "86400", num: 9 },
                                { value: "96000", num: 10 },
                                { value: "105600", num: 11 },
                                { value: "115200", num: 12 }
                            ]
                            
                            Button {
                                Layout.fillWidth: true
                                height: 55

                                scale: down ? 0.97 : 1.0
                                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                                contentItem: RowLayout {
                                    spacing: 15
                                    
                                    Rectangle {
                                        Layout.preferredWidth: 40
                                        Layout.preferredHeight: 40
                                        radius: 6
                                        color: "#01E4E0"
                                        opacity: 0.2
                                        
                                        Label {
                                            anchors.centerIn: parent
                                            text: modelData.num
                                            font.pixelSize: 18
                                            font.bold: Font.Bold
                                            font.family: "Courier New"
                                            color: "#01E4E0"
                                        }
                                    }
                                    
                                    Label {
                                        text: modelData.value + " bps"
                                        font.pixelSize: 18
                                        font.family: "Courier New"
                                        font.bold: Font.DemiBold
                                        color: "#FFFFFF"
                                        Layout.fillWidth: true
                                    }
                                }
                                
                                background: Rectangle {
                                    color: parent.down ? "#3A5A5A" : (parent.hovered ? "#2A4A4A" : "#1E3E3E")
                                    radius: 8
                                    border.color: parent.hovered ? "#01E4E0" : "#2A4A4A"
                                    border.width: 2
                                    
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                }
                                
                                onClicked: {
                                    controller.setBaudRate(modelData.num)
                                    baudDialog.close()
                                }
                            }
                        }
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "Cancel"

                    scale: down ? 0.97 : 1.0
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

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

                    onClicked: baudDialog.close()
                }
            }
        }
    }
    
    // Security Level Dialog
    Dialog {
        id: securityDialog
        modal: true
        anchors.centerIn: parent

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.9; to: 1.0; duration: 250; easing.type: Easing.OutBack }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.9; duration: 150; easing.type: Easing.InQuad }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
        }

        background: Rectangle {
            color: "#2A2A2A"
            radius: 12
            border.color: "#01E4E0"
            border.width: 2
        }
        
        contentItem: Rectangle {
            implicitWidth: 500
            implicitHeight: 450
            color: "transparent"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 25
                spacing: 15
                
                Label {
                    text: "Select Security Level"
                    font.pixelSize: 24
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
                    text: "Select fingerprint matching algorithm:"
                    font.pixelSize: 14
                    font.family: "Inter"
                    color: "#CCCCCC"
                }

                Repeater {
                    model: [
                        { level: "Lowest", desc: "Lowest security level with really fast performance", num: 1 },
                        { level: "Low", desc: "Balanced speed and security", num: 2 },
                        { level: "Balanced", desc: "Recommended default", num: 3 },
                        { level: "High", desc: "Higher security with slighter performance costs", num: 4 },
                        { level: "Highest", desc: "Maximum security level (Low FA/High FR)", num: 5 }
                    ]
                    
                    Button {
                        Layout.fillWidth: true
                        height: 60

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: ColumnLayout {
                            spacing: 4

                            Label {
                                text: modelData.level
                                font.pixelSize: 17
                                font.family: "Inter"
                                font.bold: Font.Bold
                                color: "#FFFFFF"
                            }
                            
                            Label {
                                text: modelData.desc
                                font.pixelSize: 13
                                font.family: "Inter"
                                color: "#AAAAAA"
                            }
                        }
                        
                        background: Rectangle {
                            color: parent.down ? "#3A5A5A" : (parent.hovered ? "#2A4A4A" : "#1E3E3E")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "#2A4A4A"
                            border.width: 2
                            
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        
                        onClicked: {
                            controller.setSecurityLevel(modelData.num)
                            securityDialog.close()
                        }
                    }
                }
                
                Item { Layout.fillHeight: true }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "Cancel"

                    scale: down ? 0.97 : 1.0
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

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

                    onClicked: securityDialog.close()
                }
            }
        }
    }
    
    // Packet Size Dialog
    Dialog {
        id: packetDialog
        modal: true
        anchors.centerIn: parent

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.9; to: 1.0; duration: 250; easing.type: Easing.OutBack }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.9; duration: 150; easing.type: Easing.InQuad }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
        }

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
                spacing: 15
                
                Label {
                    text: "Select Packet Size"
                    font.pixelSize: 24
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
                    text: "Select packet transmission size:"
                    font.pixelSize: 14
                    font.family: "Inter"
                    color: "#CCCCCC"
                }
                
                Repeater {
                    model: [
                        { size: "32 bytes", desc: "Smallest", num: 0 },
                        { size: "64 bytes", desc: "Small", num: 1 },
                        { size: "128 bytes", desc: "Medium (default)", num: 2 },
                        { size: "256 bytes", desc: "Large", num: 3 }
                    ]
                    
                    Button {
                        Layout.fillWidth: true
                        height: 60

                        scale: down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        contentItem: ColumnLayout {
                            spacing: 4

                            Label {
                                text: modelData.size
                                font.pixelSize: 17
                                font.family: "Inter"
                                font.bold: Font.Bold
                                color: "#FFFFFF"
                            }
                            
                            Label {
                                text: modelData.desc
                                font.pixelSize: 13
                                font.family: "Inter"
                                color: "#AAAAAA"
                            }
                        }
                        
                        background: Rectangle {
                            color: parent.down ? "#3A5A5A" : (parent.hovered ? "#2A4A4A" : "#1E3E3E")
                            radius: 8
                            border.color: parent.hovered ? "#01E4E0" : "#2A4A4A"
                            border.width: 2
                            
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        
                        onClicked: {
                            controller.setPacketSize(modelData.num)
                            packetDialog.close()
                        }
                    }
                }
                
                Item { Layout.fillHeight: true }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "Cancel"

                    scale: down ? 0.97 : 1.0
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

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

                    onClicked: packetDialog.close()
                }
            }
        }
    }
}
