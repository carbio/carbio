import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: passwordDialog
    anchors.centerIn: parent
    width: 500
    height: 350
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    // Dim background
    Overlay.modal: Rectangle {
        color: "#DD000000"
    }
    
    // Custom background with border and styling
    background: Rectangle {
        id: dialogBox
        color: "#2A2A2A"
        radius: 20
        border.color: "#01E4E0"
        border.width: 3
        
        // Open animation
        scale: passwordDialog.opened ? 1.0 : 0.8
        Behavior on scale {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutBack
            }
        }
    }
    
    signal passwordSubmitted(string password)
    signal cancelled()
    
    // Track whether dialog was closed via submission or cancellation
    property bool wasSubmitted: false
    
    // Reset state when popup is about to show (use Popup's lifecycle)
    onAboutToShow: {
        passwordField.text = ""
        wasSubmitted = false
    }
    
    onOpened: {
        passwordField.forceActiveFocus()
    }
    
    // Handle escape key and outside clicks
    // Only emit cancelled if user didn't submit password
    onClosed: {
        if (!wasSubmitted) {
            cancelled()
        }
    }
    
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25
        
        // Header with lock icon
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 15
            
            Canvas {
                width: 50
                height: 60
                
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    
                    // Lock body
                    ctx.fillStyle = "#01E4E0"
                    ctx.fillRect(10, 25, 30, 25)
                    
                    // Lock shackle
                    ctx.strokeStyle = "#01E4E0"
                    ctx.lineWidth = 4
                    ctx.lineCap = "round"
                    ctx.beginPath()
                    ctx.arc(25, 25, 12, Math.PI, 0, true)
                    ctx.stroke()
                    
                    // Keyhole
                    ctx.fillStyle = "#2A2A2A"
                    ctx.beginPath()
                    ctx.arc(25, 35, 3, 0, 2 * Math.PI)
                    ctx.fill()
                    ctx.fillRect(23, 35, 4, 8)
                }
                
                Component.onCompleted: requestPaint()
            }
            
            Label {
                text: "Admin Authorization Required"
                font.pixelSize: 20
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#01E4E0"
            }
        }
        
        // Instructions
        Label {
            Layout.fillWidth: true
            text: "Step 1 of 2: Enter admin password"
            font.pixelSize: 15
            font.family: "Inter"
            color: "#AAAAAA"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
        
        // Password field
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            color: "#1E1E1E"
            radius: 12
            border.color: passwordField.activeFocus ? "#01E4E0" : "#444444"
            border.width: 1
            
            Behavior on border.color { ColorAnimation { duration: 200 } }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 15
                
                Rectangle {
                    width: 40
                    height: 40
                    radius: 8
                    color: "transparent"
                    
                    Image {
                        id: keyIcon
                        source: "qrc:/key.svg"
                        anchors.centerIn: parent
                    }
                }
                
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    placeholderText: "Enter password..."
                    font.pixelSize: 18
                    placeholderTextColor: "#ffffff"
                    font.family: "Inter"
                    color: "#ffffff"
                    background: Rectangle { color: "transparent" }
                    
                    Keys.onReturnPressed: submitPassword()
                    Keys.onEnterPressed: submitPassword()
                    Keys.onEscapePressed: cancelDialog()
                }
                
                // Show/hide password toggle
                Rectangle {
                    width: 40
                    height: 40
                    radius: 8
                    color: toggleMouseArea.containsMouse ? "#3A3A3A" : "transparent"
                    
                    Image {
                        id: showHideIcon
                        source: passwordField.echoMode === TextInput.Password ? "qrc:/eye_show.svg" : "qrc:/eye_hide.svg"
                        anchors.centerIn: parent
                    }
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                    
                    MouseArea {
                        id: toggleMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            passwordField.echoMode = passwordField.echoMode === TextInput.Password
                                ? TextInput.Normal : TextInput.Password
                        }
                    }
                }
            }
        }
        
        // Info text
        Label {
            Layout.fillWidth: true
            text: "Default password: admin123\n(Change after first login)"
            font.pixelSize: 12
            font.family: "Inter"
            color: "#888888"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            opacity: 0.7
        }
        
        // Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 5
            spacing: 15
            
            // Cancel button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                
                background: Rectangle {
                    color: parent.hovered ? "#4A4A4A" : "#3A3A3A"
                    radius: 10
                    border.color: "#666666"
                    border.width: 2
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                
                contentItem: Label {
                    text: "Cancel"
                    font.pixelSize: 16
                    font.family: "Inter"
                    font.bold: Font.Medium
                    color: "#CCCCCC"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: cancelDialog()
            }
            
            // Submit button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                
                background: Rectangle {
                    color: parent.hovered ? "#00FFEE" : "#01E4E0"
                    radius: 10
                    border.color: "#00FFEE"
                    border.width: 2
                    
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                
                contentItem: Label {
                    text: "Continue →"
                    font.pixelSize: 16
                    font.family: "Inter"
                    font.bold: Font.Bold
                    color: "#1E1E1E"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: submitPassword()
            }
        }
    }
    
    function submitPassword() {
        if (passwordField.text.length === 0) {
            return
        }
        
        wasSubmitted = true  // Mark as submitted BEFORE closing
        passwordSubmitted(passwordField.text)
        close()
    }
    
    function cancelDialog() {
        wasSubmitted = false  // Explicitly mark as cancelled
        close()
    }
}