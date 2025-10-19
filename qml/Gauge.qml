import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: gauge
    width: 450
    height: 450
    
    property real value: 0
    property real minimumValue: 0
    property real maximumValue: 200
    property string speedColor: "#32D74B"
    property bool accelerating: false

    function speedColorProvider(val) {
        if(val < 100) {
            return "#32D74B"
        } else if(val > 100 && val < 160) {
            return "yellow"
        } else {
            return "Red"
        }
    }
    
    function valueToAngle(val) {
        var normalizedValue = (val - minimumValue) / (maximumValue - minimumValue)
        return -144 + (normalizedValue * 288)  // -144 to 144 degrees range
    }
    
    // Background circle
    Rectangle {
        id: background
        anchors.fill: parent
        color: "#1E1E1E"
        radius: width / 2
        opacity: 0.5
        
        // Arc drawing canvas
        Canvas {
            id: arcCanvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                
                var centerX = width / 2
                var centerY = height / 2
                var radius = Math.min(width, height) / 2 - 50
                
                // Draw arc based on current value
                if (gauge.value > 0) {
                    var startAngle = (-144 - 90) * Math.PI / 180
                    var endAngle = (valueToAngle(gauge.value) - 90) * Math.PI / 180
                    
                    ctx.beginPath()
                    ctx.lineWidth = radius * 0.225
                    ctx.strokeStyle = speedColorProvider(gauge.value)
                    speedColor = ctx.strokeStyle
                    ctx.arc(centerX, centerY, radius - ctx.lineWidth / 2, startAngle, endAngle)
                    ctx.stroke()
                }
            }
            
            Connections {
                target: gauge
                function onValueChanged() { arcCanvas.requestPaint() }
            }
        }
        
        // Tickmarks
        Repeater {
            model: 21 // 0 to 200 in steps of 10
            
            Item {
                id: tickmarkContainer
                anchors.centerIn: parent
                width: background.width
                height: background.height

                property real angle: -144 + (index * 288 / 20)
                property real tickValue: index * 10
                property real radius: background.width / 2 - 30

                rotation: angle

                //transform: [
                //    Translate {
                //        x: background.width / 2
                //        y: background.height / 2
                //    },
                //    Rotation {
                //        origin.x: width / 2
                //        origin.y: height / 2
                //        angle: tickmarkContainer.angle
                //    }
                //]
                
                // Major tickmark
                Rectangle {
                    //x: -width / 2
                    //y: -background.height / 2 + 30
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    //y: 30
                    width: 3
                    height: 20
                    color: tickmarkContainer.tickValue <= gauge.value ? "white" : "#777776"
                    antialiasing: true
                }
                
                // Tickmark label
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 60
                    //x: -width / 2
                    //y: -background.height / 2 + 60
                    text: tickmarkContainer.tickValue
                    font.pixelSize: Math.max(6, background.width * 0.035)
                    color: tickmarkContainer.tickValue <= gauge.value ? "white" : "#777776"
                    antialiasing: true
                    horizontalAlignment: Text.AlignHCenter
                    rotation: -tickmarkContainer.angle
                    
                    //transform: Rotation {
                    //    origin.x: width / 2
                    //    origin.y: height / 2
                    //    angle: -tickmarkContainer.angle
                    //}
                }
            }
        }
        
        // Minor tickmarks
        Repeater {
            model: 80 // More minor tickmarks

            Item {
                anchors.centerIn: parent
                width: background.width
                height: background.height

                property real angle: -144 + (index * 288 / 79)
                property real tickValue: index * 2.5

                visible: tickValue % 10 !== 0 // Hide where major tickmarks are
                
                //transform: [
                //    Translate {
                //        x: background.width / 2
                //        y: background.height / 2
                //    },
                //    Rotation {
                //        angle: angle
                //    }
                //]
                rotation: angle
                
                Rectangle {
                    //x: -width / 2
                    //y: -background.height / 2 + 35
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 35
                    width: 1.5
                    height: 10
                    color: parent.tickValue <= gauge.value ? "white" : "darkGray"
                    antialiasing: true
                }
            }
        }
        
        // Needle
        Item {
            id: needleContainer
            anchors.centerIn: parent
            visible: gauge.value > 0
            
            transform: Rotation {
                origin.x: 0
                origin.y: 0
                angle: valueToAngle(gauge.value)
                
                Behavior on angle {
                    NumberAnimation { duration: 300 }
                }
            }
            
            // Outer glow layers for depth
            Repeater {
                model: 3
                Rectangle {
                    x: -3 - index * 1.5
                    y: -background.height * 0.35 - index * 1.5
                    width: 6 + index * 3
                    height: background.height * 0.27 + index * 3
                    color: "white"
                    radius: 3 + index
                    opacity: 0.15 - index * 0.04
                    antialiasing: true
                }
            }

            // Main needle
            Rectangle {
                id: needle
                x: -2
                y: -background.height * 0.35
                width: 4
                height: background.height * 0.27
                color: "white"
                radius: 2
                antialiasing: true
            }
        }
        
        // Center display
        ColumnLayout {
            anchors.centerIn: parent
            
            Label {
                text: gauge.value.toFixed(0)
                font.pixelSize: 85
                font.family: "Inter"
                color: "#01E6DE"
                font.bold: Font.DemiBold
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: "km/h"
                font.pixelSize: 46
                font.family: "Inter"
                color: "#01E6DE"
                font.bold: Font.Normal
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
