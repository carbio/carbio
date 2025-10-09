import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import CustomControls
import "./"

ApplicationWindow {
    width: 1920
    height: 960
    visible: true
    title: qsTr("Car DashBoard")
    color: "#1E1E1E"
    visibility: Window.FullScreen
    property int nextSpeed: 60

    function generateRandom(maxLimit = 70){
        let rand = Math.random() * maxLimit;
        rand = Math.floor(rand);
        return rand;
    }

    function speedColor(value){
        if(value < 100 ){
            return "green"
        } else if(value > 100 && value < 160){
            return "yellow"
        }else{
            return "Red"
        }
    }

    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered:{
            currentTime.text = Qt.formatDateTime(new Date(), "hh:mm")
        }
    }

    Timer{
        repeat: true
        interval: 3000
        running: true
        onTriggered: {
            nextSpeed = generateRandom()
        }
    }

    Shortcut {
        sequence: "Ctrl+Q"
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Image {
        id: dashboard
        width: parent.width
        height: parent.height
        anchors.centerIn: parent
        source: "qrc:/dashboard.svg"

        // Dashboard content wrapper for smooth fade in/out transitions
        Item {
            id: dashboardContent
            anchors.fill: parent
            layer.enabled: true
            layer.smooth: true

            // Fade in when authenticated (authState === 4)
            opacity: controller.authState === 4 ? 1.0 : 0.0
            visible: opacity > 0
            enabled: controller.authState === 4

            // Smooth fade transition (slower than AuthPrompt for elegant reveal)
            Behavior on opacity {
                NumberAnimation {
                    duration: 600
                    easing.type: Easing.InOutCubic
                }
            }

        /*
          Top Bar Of Screen
        */

        Image {
            id: topBar
            width: 1357
            source: "qrc:/vector70.svg"

            anchors{
                top: parent.top
                topMargin: 26.50
                horizontalCenter: parent.horizontalCenter
            }

            Image {
                id: headLight
                property bool indicator: false
                width: 42.5
                height: 38.25
                anchors{
                    top: parent.top
                    topMargin: 25
                    leftMargin: 230
                    left: parent.left
                }
                source: indicator ? "qrc:/low_beam_headlights.svg" : "qrc:/low_beam_headlights_white.svg"
                Behavior on indicator { NumberAnimation { duration: 300 }}
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        headLight.indicator = !headLight.indicator
                    }
                }
            }


            Rectangle {
                id: gearButton
                width: 50
                height: 50
                radius: 25
                color: mouseArea.containsMouse ? "#3A3A3A" : "#2A2A2A"
                border.color: "#01E4E0"
                border.width: 2
                
                anchors {
                    verticalCenter: currentTime.verticalCenter
                    right: currentTime.left
                    rightMargin: 30
                }
                
                Behavior on color { ColorAnimation { duration: 200 } }
                Behavior on scale { NumberAnimation { duration: 100 } }
                
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onEntered: parent.scale = 1.05
                    onExited: parent.scale = 1.0
                    onPressed: parent.scale = 0.95
                    onReleased: parent.scale = mouseArea.containsMouse ? 1.05 : 1.0
                    onClicked: {
                        controller.requestAdminAccess()
                    }
                }
                
                // Gear icon using Image (no ColorOverlay needed)
                Image {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    source: "qrc:/gear.svg"
                    smooth: true
                }
            }

            Label{
                id: currentTime
                text: Qt.formatDateTime(new Date(), "hh:mm")
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.DemiBold
                color: "#FFFFFF"
                anchors.top: parent.top
                anchors.topMargin: 25
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label{
                id: currentDate
                text: Qt.formatDateTime(new Date(), "yyyy. MM. dd.")
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.DemiBold
                color: "#FFFFFF"
                anchors.right: parent.right
                anchors.rightMargin: 230
                anchors.top: parent.top
                anchors.topMargin: 25
            }
        }

        /*
          Speed Gauge
        */
        Gauge {
            id: speedLabel
            width: 450
            height: 450
            property bool accelerating
            value: accelerating ? maximumValue : 0
            maximumValue: 200

            anchors.top: parent.top
            anchors.topMargin: Math.floor(parent.height * 0.25)
            anchors.horizontalCenter: parent.horizontalCenter

            Component.onCompleted: forceActiveFocus()


            Behavior on value { NumberAnimation { duration: 1000 }}

            Keys.onSpacePressed: {
                if (controller.authState === 4) {
                    accelerating = true
                }
            }
            Keys.onReleased: {
                if (event.key === Qt.Key_Space) {
                    accelerating = false;
                    event.accepted = true;
                }else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                    radialBar.accelerating = false;
                    event.accepted = true;
                }
            }

            Keys.onEnterPressed: {
                if (controller.authState === 4) {
                    radialBar.accelerating = true
                }
            }
            Keys.onReturnPressed: {
                if (controller.authState === 4) {
                    radialBar.accelerating = true
                }
            }

        }

        /*
          Speed Limit Label
        */

        Rectangle{
            id:speedLimit
            width: 130
            height: 130
            radius: height/2
            color: "#D9D9D9"
            border.color: speedColor(maxSpeedlabel.text)
            border.width: 10

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 50

            Label{
                id:maxSpeedlabel
                text: getRandomInt(80, 130).toFixed(0)
                font.pixelSize: 45
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#01E6DE"
                anchors.centerIn: parent

                function getRandomInt(min, max) {
                    return Math.floor(Math.random() * (max - min + 1)) + min;
                }
            }
        }
        Image {
            anchors{
                bottom: car.top
                bottomMargin: 30
                horizontalCenter:car.horizontalCenter
            }
            source: "qrc:/model_3.png"
        }

        Image {
            id:car
            anchors{
                bottom: speedLimit.top
                bottomMargin: 30
                horizontalCenter:speedLimit.horizontalCenter
            }
            source: "qrc:/car.svg"
        }

        /*
          Left Road
        */

        Image {
            id: leftRoad
            width: 127
            height: 397
            anchors{
                left: speedLimit.left
                leftMargin: 100
                bottom: parent.bottom
                bottomMargin: 26.50
            }

            source: "qrc:/vector2.svg"
        }

        // Temperature display (left side, independent from speed gauge)
        RowLayout{
            spacing: 3
            anchors{
                left: parent.left
                leftMargin: 250
                bottom: parent.bottom
                bottomMargin: 26.50 + 65
            }

            Label{
                text: "38.1"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                color: "#FFFFFF"
            }

            Label{
                text: "°C"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                opacity: 0.2
                color: "#FFFFFF"
            }
        }

        /*
          Right Road
        */

        Image {
            id: rightRoad
            width: 127
            height: 397
            anchors{
                right: speedLimit.right
                rightMargin: 100
                bottom: parent.bottom
                bottomMargin: 26.50
            }

            source: "qrc:/vector1.svg"
        }

        /*
          Right Side gear
        */

        RowLayout{
            spacing: 20
            anchors{
                right: parent.right
                rightMargin: 350
                bottom: parent.bottom
                bottomMargin: 26.50 + 65
            }

            Label{
                text: "Ready"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                color: "#32D74B"
            }

            Label{
                text: "P"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                color: "#FFFFFF"
            }

            Label{
                text: "R"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                opacity: 0.2
                color: "#FFFFFF"
            }
            Label{
                text: "N"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                opacity: 0.2
                color: "#FFFFFF"
            }
            Label{
                text: "D"
                font.pixelSize: 32
                font.family: "Inter"
                font.bold: Font.Normal
                font.capitalization: Font.AllUppercase
                opacity: 0.2
                color: "#FFFFFF"
            }
        }

        /*Left Side Icons*/
        Image {
            id:forthLeftIndicator
            property bool parkingLightOn: true
            width: 72
            height: 62
            anchors{
                left: parent.left
                leftMargin: 175
                bottom: thirdLeftIndicator.top
                bottomMargin: 25
            }
            Behavior on parkingLightOn { NumberAnimation { duration: 300 }}
            source: parkingLightOn ? "qrc:/parking_lights.svg" : "qrc:/parking_lights_white.svg"
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    forthLeftIndicator.parkingLightOn = !forthLeftIndicator.parkingLightOn
                }
            }
        }

        Image {
            id:thirdLeftIndicator
            property bool lightOn: true
            width: 52
            height: 70.2
            anchors{
                left: parent.left
                leftMargin: 145
                bottom: secondLeftIndicator.top
                bottomMargin: 25
            }
            source: lightOn ? "qrc:/lights.svg" : "qrc:/light_white.svg"
            Behavior on lightOn { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    thirdLeftIndicator.lightOn = !thirdLeftIndicator.lightOn
                }
            }
        }

        Image {
            id:secondLeftIndicator
            property bool headLightOn: true
            width: 51
            height: 51
            anchors{
                left: parent.left
                leftMargin: 125
                bottom: firstLeftIndicator.top
                bottomMargin: 30
            }
            Behavior on headLightOn { NumberAnimation { duration: 300 }}
            source:headLightOn ?  "qrc:/low_beam_headlights.svg" : "qrc:/low_beam_headlights_white.svg"

            MouseArea{
                anchors.fill: parent
                onClicked: {
                    secondLeftIndicator.headLightOn = !secondLeftIndicator.headLightOn
                }
            }
        }

        Image {
            id:firstLeftIndicator
            property bool rareLightOn: false
            width: 51
            height: 51
            anchors{
                left: parent.left
                leftMargin: 100
                verticalCenter: speedLabel.verticalCenter
            }
            source: rareLightOn ? "qrc:/rare_fog_lights_red.svg" : "qrc:/rare_fog_lights.svg"
            Behavior on rareLightOn { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    firstLeftIndicator.rareLightOn = !firstLeftIndicator.rareLightOn
                }
            }
        }

        /*Right Side Icons*/

        Image {
            id:forthRightIndicator
            property bool indicator: true
            width: 56.83
            height: 36.17
            anchors{
                right: parent.right
                rightMargin: 195
                bottom: thirdRightIndicator.top
                bottomMargin: 50
            }
            source: indicator ? "qrc:/fourth_right_icon.svg" : "qrc:/fourth_right_icon_red.svg"
            Behavior on indicator { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    forthRightIndicator.indicator = !forthRightIndicator.indicator
                }
            }
        }

        Image {
            id:thirdRightIndicator
            property bool indicator: true
            width: 56.83
            height: 36.17
            anchors{
                right: parent.right
                rightMargin: 155
                bottom: secondRightIndicator.top
                bottomMargin: 50
            }
            source: indicator ? "qrc:/third_right_icon.svg" : "qrc:/third_right_icon_red.svg"
            Behavior on indicator { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    thirdRightIndicator.indicator = !thirdRightIndicator.indicator
                }
            }
        }

        Image {
            id:secondRightIndicator
            property bool indicator: true
            width: 56.83
            height: 36.17
            anchors{
                right: parent.right
                rightMargin: 125
                bottom: firstRightIndicator.top
                bottomMargin: 50
            }
            source: indicator ? "qrc:/second_right_icon.svg" : "qrc:/second_right_icon_red.svg"
            Behavior on indicator { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    secondRightIndicator.indicator = !secondRightIndicator.indicator
                }
            }
        }

        Image {
            id:firstRightIndicator
            property bool sheetBelt: true
            width: 36
            height: 45
            anchors{
                right: parent.right
                rightMargin: 100
                verticalCenter: speedLabel.verticalCenter
            }
            source: sheetBelt ? "qrc:/first_right_icon.svg" : "qrc:/first_right_icon_grey.svg"
            Behavior on sheetBelt { NumberAnimation { duration: 300 }}
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    firstRightIndicator.sheetBelt = !firstRightIndicator.sheetBelt
                }
            }
        }

        // Progress Bar
        RadialBar {
            id:radialBar
            anchors{
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: parent.width / 6
            }

            width: 338
            height: 338
            penStyle: Qt.RoundCap
            dialType: RadialBar.NoDial
            progressColor: "#01E4E0"
            backgroundColor: "transparent"
            dialWidth: 17
            startAngle: 270
            spanAngle: 3.6 * value
            minValue: 0
            maxValue: 100
            value: accelerating ? maxValue : 65
            textFont {
                family: "inter"
                italic: false
                bold: Font.Medium
                pixelSize: 60
            }
            showText: false
            suffixText: ""
            textColor: "#FFFFFF"


            property bool accelerating
            Behavior on value { NumberAnimation { duration: 1000 }}

            ColumnLayout{
                anchors.centerIn: parent
                Label{
                    text: radialBar.value.toFixed(0) + "%"
                    font.pixelSize: 65
                    font.family: "Inter"
                    font.bold: Font.Normal
                    color: "#FFFFFF"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label{
                    text: "Battery charge"
                    font.pixelSize: 28
                    font.family: "Inter"
                    font.bold: Font.Normal
                    opacity: 0.8
                    color: "#FFFFFF"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        ColumnLayout{
            spacing: 40

            anchors{
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: parent.width / 6
            }


            RowLayout{
                spacing: 30
                Image {
                    width: 72
                    height: 50
                    source: "qrc:/road.svg"
                }

                ColumnLayout{
                    Label{
                        text: "188 KM"
                        font.pixelSize: 30
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                    Label{
                        text: "Distance"
                        font.pixelSize: 20
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                }
            }
            RowLayout{
                spacing: 30
                Image {
                    width: 72
                    height: 78
                    source: "qrc:/fuel.svg"
                }

                ColumnLayout{
                    Label{
                        text: "6.9 L/100km"
                        font.pixelSize: 30
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                    Label{
                        text: "Avg. Fuel Usage"
                        font.pixelSize: 20
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                }
            }
            RowLayout{
                spacing: 30
                Image {
                    width: 72
                    height: 72
                    source: "qrc:/speedometer.svg"
                }

                ColumnLayout{
                    Label{
                        text: "125 km/h"
                        font.pixelSize: 30
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                    Label{
                        text: "Avg. Speed"
                        font.pixelSize: 20
                        font.family: "Inter"
                        font.bold: Font.Normal
                        opacity: 0.8
                        color: "#FFFFFF"
                    }
                }
            }
        }
        }

        // Authentication overlay (sits on top, controls access)
        AuthPrompt {
            id: authPrompt
            anchors.fill: parent
            authState: controller.authState
            failedAttempts: controller.failedAttempts
            lockoutSeconds: controller.lockoutSeconds
            driverName: controller.driverName
            isProcessing: controller.isProcessing
            z: 100
            onAuthPromptHiding: {}

            Connections {
                target: controller
                function onAuthenticationSuccess(driverName) {
                    authPrompt.showSuccess(driverName)
                }
                function onAuthenticationFailed() {
                    authPrompt.showFailure(controller.failedAttempts)
                }
            }
        }

        Toast {
            id: toast
            z: 300
        }

        AdminPasswordDialog {
            id: adminPasswordDialog
            onPasswordSubmitted: function(password) {
                controller.verifyAdminPassword(password)
            }
            onCancelled: {
                controller.revokeAdminAccess()
            }
        }

        // Fingerprint verification prompt
        InfoDialog {
            id: fingerprintDialog
        }

        UnauthorizedAccessWarning {
            id: unauthorizedAccessWarning
            onAcknowledged: {
                controller.revokeAdminAccess()
            }
        }

        FingerprintSetupDialog {
            id: fingerprintSetupDialog
            //anchors.fill: parent
            //anchors.centerIn: parent
            //z: 100
        }

        // Info dialog for detailed messages
        InfoDialog {
            id: infoDialog
            //z: 350
        }

        Connections {
            target: controller

            function onOperationComplete(message) {
                if (message.includes("\n") || message.includes("Template #") || message.length > 80) {
                    var isSystemInfo = message.includes("System Settings:")
                    var isQueryResult = message.includes("Template #")
                    var title = isSystemInfo ? "System Configuration" :
                               (isQueryResult ? "Query Result" : "Information")
                    infoDialog.show(title, message, false)
                } else {
                    // Short messages use toast
                    toast.show(message, false)
                }
            }

            function onOperationFailed(error) {
                // Errors always use toast for quick dismissal
                toast.show(error, true)
            }

            function onAdminPasswordRequired() {
                adminPasswordDialog.open()
            }

            function onAdminPasswordFailed(remainingAttempts) {
                toast.show("Incorrect password. " + remainingAttempts + " attempts remaining.", true)
                if (remainingAttempts > 0) {
                    adminPasswordDialog.open()
                }
            }

            function onAdminFingerprintRequired() {
                fingerprintDialog.show("Biometric Verification",
                    "Step 2 of 2: Place your admin fingerprint on the sensor...",
                    false)
                controller.startAdminFingerprintScan()
            }

            function onAdminAccessGranted(token) {
                fingerprintDialog.show("Access Granted", "Admin access granted. Opening settings...", false)
                // Show menu after short delay
                adminAccessTimer.start()
            }

            function onAdminAccessDenied(reason) {
                fingerprintDialog.show("Access Denied", reason, true)
            }

            function onUnauthorizedAccessDetected(details) {
                // Close any open dialogs
                adminPasswordDialog.close()
                fingerprintDialog.close()
                // Show warning
                unauthorizedAccessWarning.show(details)
            }
            function onAdminAccessRevoked() {
                adminPasswordDialog.close()
                fingerprintDialog.close()
                infoDialog.close()
                unauthorizedAccessWarning.close()
                fingerprintSetupDialog.close()
            }
        }

        Timer {
            id: adminAccessTimer
            interval: 1500
            running: false
            repeat: false
            onTriggered: {
                fingerprintSetupDialog.open()
                fingerprintDialog.close()
            }
        }
    }

    // Close button
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        width: 50
        height: 50
        radius: 25
        color: "#2A2A2A"
        border.color: "#444444"
        border.width: 2
        opacity: 0.7
        z: 1000

        Behavior on opacity { NumberAnimation { duration: 200 } }
        Behavior on color { ColorAnimation { duration: 200 } }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onEntered: {
                parent.opacity = 1.0
                parent.color = "#3A3A3A"
            }

            onExited: {
                parent.opacity = 0.7
                parent.color = "#2A2A2A"
            }

            onPressed: {
                parent.color = "#FF3333"
                parent.scale = 0.95
            }

            onReleased: {
                parent.scale = 1.0
            }

            onClicked: {
                Qt.quit()
            }
        }

        // X mark
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = "#CCCCCC"
                ctx.lineWidth = 3
                ctx.lineCap = "round"

                var margin = 15
                var size = parent.width - margin * 2

                // Draw X
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

        Behavior on scale { NumberAnimation { duration: 100 } }
    }
}